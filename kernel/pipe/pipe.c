#include <os/mm.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/irq.h>
#include <pgtable.h>
#include <os/list.h>
#include <os/pipe.h>
#include <os/swap.h>

static pipe_t pipes[PIPE_MAX_NUM];

void init_pipes() {
    for (int i = 0; i < PIPE_MAX_NUM; i++) {
        pipes[i].valid = 0;
        init_queue(&pipes[i].wait_queue);
    }
}

int sys_pipe_open(const char *name) {
    int i;
    int free_idx = -1;

    // 查找已存在的管道
    for (i = 0; i < PIPE_MAX_NUM; i++) {
        if (pipes[i].valid && strcmp(pipes[i].name, name) == 0) {
            return i;
        }
        if (!pipes[i].valid && free_idx == -1) {
            free_idx = i; // 同时记录空闲管道
        }
    }

    // 若没找到，创建新管道
    if (free_idx != -1) {
        strcpy(pipes[free_idx].name, name);
        pipes[free_idx].valid = 1;
        pipes[free_idx].head = 0;
        pipes[free_idx].tail = 0;
        pipes[free_idx].data_count = 0;
        init_queue(&pipes[free_idx].wait_queue);
        return free_idx;
    }
    return -1; // 失败
}

// 发送方：交出物理页 (Unmap)
long sys_pipe_give_pages(int pipe_idx, void *src, size_t length) {
    if (pipe_idx < 0 || pipe_idx >= PIPE_MAX_NUM || !pipes[pipe_idx].valid) return -1;
    // 必须页对齐
    if ((uintptr_t)src % PAGE_SIZE != 0 || length % PAGE_SIZE != 0) return -1; 

    pipe_t *pipe = &pipes[pipe_idx];
    int pages_total = length / PAGE_SIZE; // 需要发送的总页数
    int pages_done = 0; // 已发送页数
    uintptr_t current_va = (uintptr_t)src;

    while (pages_done < pages_total) {
        // 缓冲区满则阻塞等待
        if (pipe->data_count >= PIPE_BUFFER_SIZE) {
            current_running->status = TASK_BLOCKED;
            enqueue(&current_running->list, &pipe->wait_queue);
            do_scheduler();
            continue; // 醒来后重试
        }

        // 获取源地址的页表项
        PTE *pte = get_pte(current_va, current_running->pgdir_kva);
        uintptr_t pa = 0;

        // 页表项有效，在内存中
        if(pte && (*pte & _PAGE_PRESENT)){
            // 若为共享页，不允许发送
            if (*pte & _PAGE_SHM) return -1;

            // 普通私有页
            pa = (*pte >> _PAGE_PFN_SHIFT) << NORMAL_PAGE_SHIFT; // 从页表项提取pa
            // 管道页面不应该被swap，从active_pages中移除
            remove_from_active_list(pa2kva(pa));
            // 解除映射
            *pte = 0; 
            local_flush_tlb_page(current_va);
        }else if (pte && (*pte & _PAGE_SWAP)) {
        // 页面被换出到磁盘
            int swap_id = (*pte) >> _PAGE_PFN_SHIFT;
            ptr_t kva = allocPage(1); // 分配一页（无需记录到active_pages中）
            pa = kva2pa(kva);
            // 换回内存
            swap_in(kva, swap_id);
            *pte = 0;
            // 不需要 flush TLB，因为本来就无效
        }else{
            // 无效内存
            return -1; // Segmentation Fault
        }

        // 将 pa 放入管道
        pipe->page_buffer[pipe->head] = pa;
        pipe->head = (pipe->head + 1) % PIPE_BUFFER_SIZE;
        pipe->data_count++;

        // 唤醒接收者
        if (!is_empty_queue(&pipe->wait_queue)) {
            wakeup(&pipe->wait_queue);
        }

        current_va += PAGE_SIZE;
        pages_done++;
    }
    return length;
}

// 接收方：获取物理页 (Map)
long sys_pipe_take_pages(int pipe_idx, void *dst, size_t length) {
    if (pipe_idx < 0 || pipe_idx >= PIPE_MAX_NUM || !pipes[pipe_idx].valid) return -1;
    if ((uintptr_t)dst % PAGE_SIZE != 0 || length % PAGE_SIZE != 0) return -1;

    pipe_t *pipe = &pipes[pipe_idx];
    int pages_total = length / PAGE_SIZE;
    int pages_done = 0;
    uintptr_t current_va = (uintptr_t)dst;

    while (pages_done < pages_total) {
        // 缓冲区空则阻塞等待
        if (pipe->data_count == 0) {
            current_running->status = TASK_BLOCKED;
            enqueue(&current_running->list, &pipe->wait_queue);
            do_scheduler();
            continue;
        }

        // 从管道取出 PA
        uintptr_t pa = pipe->page_buffer[pipe->tail];
        pipe->tail = (pipe->tail + 1) % PIPE_BUFFER_SIZE;
        pipe->data_count--;

        // 建立映射
        // 这里不加 _PAGE_SHM，因为接收方拥有该页后，它就是普通的私有页
        map_page(current_va, pa, current_running->pgdir_kva, 
                 _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC | _PAGE_USER);
        // 将该页加入 Active List，使其可被 Swap
        add_to_active_list(pa, current_va, current_running->pid);

        // 唤醒发送者
        if (!is_empty_queue(&pipe->wait_queue)) {
            wakeup(&pipe->wait_queue);
        }

        current_va += PAGE_SIZE;
        pages_done++;
    }
    return length;
}