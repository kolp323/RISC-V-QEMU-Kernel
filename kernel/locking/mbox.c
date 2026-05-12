#include <os/lock.h>
#include <os/list.h>
#include <os/sched.h>
#include <os/proc.h>
#include <printk.h>
#include <os/irq.h>
#include <os/string.h>
#include <os/mm.h>
#include <os/swap.h>

mailbox_t mailboxes[MBOX_NUM];

void init_mbox(){
    for (int i = 0; i < MBOX_NUM; i++) {
        spin_lock_init(&mailboxes[i].lock);
        *mailboxes[i].name = '\0';
        mailboxes[i].user_count = 0;
        init_queue(&mailboxes[i].recv_queue);
        init_queue(&mailboxes[i].send_queue);
        mailboxes[i].nread = mailboxes[i].nwrite = 0;
    }
}
int do_mbox_open(char *name){
    // 分配资源：全局锁
    
    for (int i = 0; i < MBOX_NUM; i++) {
        if (mailboxes[i].user_count > 0 && strcmp(mailboxes[i].name, name) == 0) {
            mailboxes[i].user_count++; // 增加引用计数
            
            return i;
        }
    }
    for (int i = 0; i < MBOX_NUM; i++) {
        if (mailboxes[i].user_count == 0) {
            strcpy(mailboxes[i].name, name); // 初始化name
            mailboxes[i].user_count = 1; // 引用计数设为1
            
            return i;
        }
    }
    
    return -1; // 无空闲信箱
}
void do_mbox_close(int mbox_idx){
    // 检查索引有效性
    if (mbox_idx < 0 || mbox_idx >= MBOX_NUM) {
        printl("[Warning] do_mbox_close: invalid mbox_idx %d\n", mbox_idx);
        return; 
    }
    mailbox_t* mailbox = &mailboxes[mbox_idx];
    // 涉及资源的销毁（减少引用或摧毁整个邮箱）
    
    // 保证了临界区内不会有其他进程开邮箱
    mailbox->user_count--;
    // 无引用时摧毁邮箱，name可以保留
    if (mailbox->user_count == 0) {
        // 此时不可能有进程再使用邮箱：引用数为0，且不能新开
        mailbox->nread = mailbox->nwrite = 0;
        // 防止永久阻塞
        wakeup(&mailbox->recv_queue);
        wakeup(&mailbox->send_queue);
    }
    
}

// 若该页有效，返回原本的kva
// 若无效，返回新页的kva
uintptr_t check_one_page(uintptr_t va){
    uintptr_t pgdir_kva = current_running->pgdir_kva;
    PTE *pte = get_pte(va, pgdir_kva); 
    if (pte != NULL && get_attribute(*pte, _PAGE_PRESENT)) {
        // 有效
        return pa2kva(get_pa(*pte));
    }
    // 无效
    return handle_page_fault(NULL, (uint64_t)va, 0);
}


void check_and_pin_pages(uintptr_t start_va, int len){
    uintptr_t cur = ROUNDDOWN(start_va, PAGE_SIZE);
    uintptr_t end = ROUNDDOWN(start_va + len - 1, PAGE_SIZE);
    uintptr_t page_kva = 0;
    for (; cur <= end; cur += PAGE_SIZE) {
        page_kva = check_one_page(cur);
        if(page_kva) remove_from_active_list(page_kva); // pin
    }
}

int do_mbox_send(int mbox_idx, void * msg, int msg_length){

    // check_pages((uintptr_t)msg, (msg_length + PAGE_SIZE - 1)/PAGE_SIZE);
    // 检查并pin住带待写页
    printl("[do_mbox_send] check and pin pages from %lx\n", msg);
    // check_and_pin_pages((uintptr_t)msg, msg_length);

    // 检查索引有效性
    if (mbox_idx < 0 || mbox_idx >= MBOX_NUM) {
        printl("[Warning] do_mbox_send: invalid mbox_idx %d\n", mbox_idx);
        return -1; 
    }
    mailbox_t* mailbox = &mailboxes[mbox_idx];
    // 检查引用计数（确保邮箱仍然有效）
    if (mailbox->user_count == 0) {
        return -1; // Mailbox 已关闭
    }

    char *data = (char *)msg;
    int bytes_sent = 0;

    // 先检查第一页
    uintptr_t last_pinned_page_kva = check_one_page((uintptr_t)data); // 上次被pin页的kva
    remove_from_active_list(last_pinned_page_kva); // pin住首页
    int unpin_last = 0; // 是否在写结束后unpin本页并pin住下一页
    uintptr_t last_pinned_page_uva = ROUNDDOWN(msg, PAGE_SIZE); // 上次pin住页的用户虚地址

    // 使用资源：内部锁
    spin_lock_acquire(&mailbox->lock);
    // 外层循环：直到发送完所有数据
    while (bytes_sent < msg_length) {
        // 内层循环：等待可用空间
        while (1) {
            int used = mailbox->nwrite - mailbox->nread;
            int remain_space = MAX_MBOX_LENGTH - used;
            // 有空间，跳出等待去写入
            if (remain_space > 0) {
                break; 
            }

            // 空间不足：阻塞等待
            disable_preempt();
            do_block(&current_running->list, &mailbox->send_queue);
            spin_lock_release(&mailbox->lock);
            enable_preempt();
            do_scheduler();
            // 醒来后检查自己是否被杀死
            if (current_running->killed) {
                do_mbox_close(mbox_idx);
                do_exit(); 
            }
            spin_lock_acquire(&mailbox->lock);
        }
        // 计算本次能写入多少数据
        int used = mailbox->nwrite - mailbox->nread;
        int remain_space = MAX_MBOX_LENGTH - used;
        int bytes_left = msg_length - bytes_sent;
        int chunk_size = (remain_space < bytes_left) ? remain_space : bytes_left;
        // 页对齐:，不许跨页传输
        int next_page_boundry = ROUNDDOWN(bytes_sent + PAGE_SIZE, PAGE_SIZE); // 下一页的起始字节数
        int max_allow_send = next_page_boundry - bytes_sent;
        if (chunk_size >= max_allow_send) { // 这次会写满本页
            chunk_size = max_allow_send;
            unpin_last = 1;
        }
        // 写入 chunk_size 大小的数据
        for (int i = 0; i < chunk_size; i++) {
            mailbox->buf[mailbox->nwrite % MAX_MBOX_LENGTH] = data[bytes_sent + i];
            mailbox->nwrite++;
        }
        // unpin 上一页， pin住下一页
        if (unpin_last) {
            // unpin 上一页
            add_to_active_list(kva2pa(last_pinned_page_kva), last_pinned_page_uva, current_running->pid);
            // pin 下一页
            last_pinned_page_uva += PAGE_SIZE;
            last_pinned_page_kva = check_one_page(last_pinned_page_uva);
            remove_from_active_list(last_pinned_page_kva);
            unpin_last = 0;
        }
        
        bytes_sent += chunk_size;
        printl("Mbox %d has sent %x bytes \n", mbox_idx, bytes_sent);
        // 每次写入一部分后，都要唤醒接收者，因为可能有数据可读了
        wakeup(&mailbox->recv_queue);
    }
    spin_lock_release(&mailboxes[mbox_idx].lock);
    return bytes_sent;
}

int do_mbox_recv(int mbox_idx, void * msg, int msg_length){

    // check_pages((uintptr_t)msg, (msg_length + PAGE_SIZE - 1)/PAGE_SIZE);
    printl("[do_mbox_recv] check and pin pages from %lx\n", msg);
    // check_and_pin_pages((uintptr_t)msg, msg_length);

    // 检查索引有效性
    if (mbox_idx < 0 || mbox_idx >= MBOX_NUM) {
        printl("[Warning] do_mbox_recv: invalid mbox_idx %d\n", mbox_idx);
        return -1; 
    }
    mailbox_t* mailbox = &mailboxes[mbox_idx];
    // 检查引用计数（确保邮箱仍然有效）
    if (mailbox->user_count == 0) {
        return -1; // Mailbox 已关闭
    }

    char *data = (char *)msg;
    int bytes_read = 0;

    // 先检查第一页
    uintptr_t last_pinned_page_kva = check_one_page((uintptr_t)data); // 上次被pin页的kva
    remove_from_active_list(last_pinned_page_kva); // pin住首页
    int unpin_last = 0; // 是否在写结束后unpin上一页
    uintptr_t last_pinned_page_uva = ROUNDDOWN(msg, PAGE_SIZE); // 上次pin住页的用户虚地址


    spin_lock_acquire(&mailbox->lock);
    while (bytes_read < msg_length) {
        // 内层循环：等待有数据可读
        while (1) {
            int available_data = mailbox->nwrite - mailbox->nread;
            // 有数据，跳出等待去读取
            if (available_data > 0) {
                break;
            }
            // 数据不足：阻塞等待
            disable_preempt();
            do_block(&current_running->list, &mailbox->recv_queue);
            spin_lock_release(&mailbox->lock);
            enable_preempt();
            do_scheduler();
            // 醒来后检查自己是否被杀死
            if (current_running->killed) {
                do_mbox_close(mbox_idx);
                do_exit(); 
            }
            spin_lock_acquire(&mailbox->lock);
        }

        int available_data = mailbox->nwrite - mailbox->nread; // 本次最多可读取的数据量
        int bytes_left = msg_length - bytes_read; // 剩余未读取的数据量
        // 实际要读多少数据
        // 能完成任务直接读完；不能则有多少读多少
        int chunk_size = (bytes_left <= available_data) ? bytes_left : available_data;

        // 页对齐:，不许跨页传输
        int next_page_boundry = ROUNDDOWN(bytes_read + PAGE_SIZE, PAGE_SIZE); // 下一页的起始字节数
        int max_allow_send = next_page_boundry - bytes_read;
        if (chunk_size >= max_allow_send) { // 这次会写满本页
            chunk_size = max_allow_send;
            unpin_last = 1;
        }

        // 读取数据
        for (int i = 0; i < chunk_size; i++) {
            data[bytes_read + i] = mailbox->buf[mailbox->nread % MAX_MBOX_LENGTH];
            mailbox->nread++;
        }

        // unpin 上一页， pin住下一页
        if (unpin_last) {
            // unpin 上一页
            add_to_active_list(kva2pa(last_pinned_page_kva), last_pinned_page_uva, current_running->pid);
            // pin 下一页
            last_pinned_page_uva += PAGE_SIZE;
            last_pinned_page_kva = check_one_page(last_pinned_page_uva);
            remove_from_active_list(last_pinned_page_kva);
            unpin_last = 0;
        }

        bytes_read += chunk_size;
        printl("Mbox %d has read %x bytes \n", mbox_idx, bytes_read);
        // 每次读取一部分后，都要唤醒发送者，因为有空间可写了
        wakeup(&mailbox->send_queue);
    }

    spin_lock_release(&mailboxes[mbox_idx].lock);
    return bytes_read;
}