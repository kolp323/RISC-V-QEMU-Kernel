#include <os/sched.h>
#include <os/list.h>
#include <os/lock.h>
#include <os/time.h>
#include <os/mm.h>
#include <screen.h>
#include <printk.h>
#include <csr.h>
#include <os/string.h>
#include <os/task.h>
#include <os/loader.h>
#include <os/proc.h>
#include <os/smp.h>
#include <os/irq.h>

static void init_thread_stack(
    ptr_t kernel_stack, ptr_t thread_stack_kva, ptr_t thread_stack_uva, 
    ptr_t entry_point, pcb_t *pcb, ptr_t arg, int arg_bytes
){
    // 保存参数
    char* arg_base_kva = (char*)thread_stack_kva - arg_bytes;
    memcpy((uint8_t *)arg_base_kva, (uint8_t *)arg, arg_bytes);

    ptr_t new_user_sp_kva = (ptr_t)arg_base_kva & ~0xF; // 向下对齐到 16 字节边界
    size_t total_offset = thread_stack_kva - new_user_sp_kva;
    ptr_t new_user_sp_uva = thread_stack_uva - total_offset;

    size_t arg_offset = thread_stack_kva - (ptr_t)arg_base_kva;
    ptr_t arg_base_uva = thread_stack_uva - arg_offset;

     regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    pt_regs->sepc = entry_point;
    pt_regs->sstatus = SR_SPIE; 
    pt_regs->regs[2] = new_user_sp_uva; // 用户栈 （注意：实际恢复时直接用pcb->user_sp）
    pt_regs->regs[4] = (reg_t) pcb; // tp
    pt_regs->regs[10] = (reg_t) arg_base_uva; // a0: 传入arg
    printl("[full_user_context] at 0x%lx\n\tsepc: 0x%lx, sstatus: 0x%lx, sp: 0x%lx, tp: 0x%lx\n",
        pt_regs,
        pt_regs->sepc,
        pt_regs->sstatus,
        pt_regs->regs[2],
        pt_regs->regs[4]
    );

    switchto_context_t *pt_switchto =
        (switchto_context_t *)((ptr_t)pt_regs - sizeof(switchto_context_t));
    // 制作假现场
    pt_switchto->regs[0] = (reg_t) ret_from_exception; // ra
    pt_switchto->regs[1] = (reg_t) pt_regs; // pt_switchto中存储的sp指向完整上下文pt_regs，需要在switch_to中恢复
    pcb->kernel_sp = (reg_t) pt_switchto; 
    pcb->user_sp = new_user_sp_uva;
    pcb->full_context = (reg_t) pt_regs;   
}
// 注意传入的参数需要为栈顶地址
static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack_kva, ptr_t entry_point,
    pcb_t *pcb, int argc, char **argv)
{

    // 将命令行参数压入栈中
    ptr_t *argv_base_kva = (ptr_t *)user_stack_kva - argc; // 向上填充argv指针数组
    ptr_t *argv_base_uva = (ptr_t *)USER_STACK_ADDR - argc; 
    char* arg_base_kva = (char *)argv_base_kva; // 向下填充字符串
    char* arg_base_uva = (char *)argv_base_uva; // 和kva同步偏移，最后得到用户栈顶的用户空间虚地址
    for (int i = 0; i < argc; i++) {
        int arg_len = strlen(argv[i]); // 单位B
        arg_base_kva -= (arg_len + 1); //为参数*(argv[i])分配空间
        arg_base_uva -= (arg_len + 1); 
        strcpy(arg_base_kva, argv[i]); // 复制参数到栈上
        argv_base_kva[i] = (ptr_t) arg_base_uva; // 令栈上的argv[i]指向对应参数（需要填充用户虚地址）
        printl("argv[%d]: 0x%lx\n", i, arg_base_uva);
    }
    printl("argv_base_kva: 0x%lx\n", argv_base_kva);
    // RISC-V 的 ABI[2] 要求栈指针的地址是 128 bit 对齐的

    ptr_t new_user_sp_uva = (ptr_t)arg_base_uva & ~0xF;

    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    pt_regs->sepc = entry_point;
    pt_regs->sstatus = SR_SPIE; 
    pt_regs->regs[2] = new_user_sp_uva; // 用户栈 （注意：实际恢复时直接用pcb->user_sp）
    pt_regs->regs[4] = (reg_t) pcb; // tp
    pt_regs->regs[10] = (reg_t) argc; // a0: 传入argc
    pt_regs->regs[11] = (reg_t) argv_base_uva; // a1: 指向栈中的argv
    printl("[full_user_context] at 0x%lx\n\tsepc: 0x%lx, sstatus: 0x%lx, sp: 0x%lx, tp: 0x%lx\n",
        pt_regs,
        pt_regs->sepc,
        pt_regs->sstatus,
        pt_regs->regs[2],
        pt_regs->regs[4]
    );

    switchto_context_t *pt_switchto =
        (switchto_context_t *)((ptr_t)pt_regs - sizeof(switchto_context_t));
    // 制作假现场
    pt_switchto->regs[0] = (reg_t) ret_from_exception; // ra
    pt_switchto->regs[1] = (reg_t) pt_regs; // pt_switchto中存储的sp指向完整上下文pt_regs，需要在switch_to中恢复
    pcb->kernel_sp = (reg_t) pt_switchto; 
    pcb->user_sp = new_user_sp_uva; // 用户虚拟地址
    pcb->full_context = (reg_t) pt_regs;
    printl("[switchto_context] at 0x%lx\n\tra: 0x%lx, sp: 0x%lx\n", pt_switchto, pt_switchto->regs[0], pt_switchto->regs[1]);
}


pid_t do_exec(char *name, int argc, char **argv){
    printl("exec task %s ...\n", name);
    printl("argc= %d, ", argc);
    printl("argv={");
    if (argc > 0) printl("\"%s\"", argv[0]);
    for (int i = 1; i < argc; i++)
        printl(", \"%s\"", argv[i]);
    printl("}\n");

    // 初始化pcb
    // 寻找空闲的pcb资源
    int idx = 0;
    for (; idx < TASK_MAXNUM; idx++) {
        if (pcb[idx].status == TASK_EXITED) break;
    }

    if (idx == TASK_MAXNUM) {
        printl("[ERROR] No more available pcb space!\n\n");
        return 0; // 执行失败
    }  
    pcb[idx].pid = process_id++;
    pcb[idx].thread_group_id = 0; // 主进程thread_group_id设置为0，与线程区分开来
    pcb[idx].status = TASK_READY; // 先设置状态，防止被分配给别的进程
    pcb[idx].killed = 0; // 复活
    pcb[idx].mask = current_running->mask; // 默认继承父进程的mask，如果本命令有设置，回到do_taskset后会被强制覆盖
    pcb[idx].dir_inode = current_running->dir_inode;
    //[p4-task1]
    // 分配根页表 (PGDIR)
    ptr_t pgdir_kva = allocPage(1); // allocPage 返回 KVA
    // 将内核页表拷贝到用户页目录 (共享内核空间映射)
    share_pgtable(pgdir_kva, pa2kva(PGDIR_PA)); // PGDIR_PA 是内核根页表的物理地址
    pcb[idx].pgdir_kva = pgdir_kva; // 保存页目录 KVA

    // 加载任务
    ptr_t entry_point = load_task_img(name, pgdir_kva, pcb[idx].pid);
    if(entry_point == 0){
        printl("[ERROR] Invalid taskname!\n\n");
        freePage(pgdir_kva); // 释放刚才分配的页表空间
        return 0; // 执行失败
    }
    local_flush_icache_all(); // 刷新指令缓存
    
    // 设置用户栈的虚拟地址 (USER_STACK_ADDR - mm.h)
    uintptr_t user_stack_base_va = USER_STACK_ADDR - PAGE_SIZE;
    // 分配物理页并映射到用户栈 va
    uintptr_t user_stack_base_kva = alloc_page_helper(user_stack_base_va, pcb[idx].pgdir_kva, pcb[idx].pid);
    // 将其存储在 PCB 中，用于后续的栈上下文制作
    // pcb[idx].user_stack_base = user_stack_va;


    init_pcb_stack(
        pcb[idx].kernel_stack_base + PAGE_SIZE * KERNEL_PAGE_NUM,
        user_stack_base_kva + PAGE_SIZE, 
        (ptr_t )entry_point, &pcb[idx], argc, argv);
        
    init_queue(&pcb[idx].list);
    init_queue(&pcb[idx].wait_list);
    printl("pid= %d, ", pcb[idx].pid);
    strcpy(pcb[idx].name, name);

    enqueue(&pcb[idx].list, &ready_queue); // 加入就绪队列
    printl("pid: %d, kernel_sp: 0x%lx, user_sp: 0x%lx\n\n", 
        pcb[idx].pid, pcb[idx].kernel_stack_base, pcb[idx].user_stack_base);
    return pcb[idx].pid;
}

// 由pid获取相应的pcb，约定：需要由调用者获取和释放锁
static pcb_t *get_pcb(pid_t pid)
{
    for (int i = 0; i < TASK_MAXNUM; i++) {
        if (pcb[i].pid == pid && pcb[i].status != TASK_EXITED && pcb[i].killed != 1) {
            return &pcb[i];
        }
    }
    return NULL;
}

// 约定(继承自do_unblock)：调用者应该保护唤醒队列
void wakeup(list_head *queue){
    list_node_t *node = queue->next;
    while (node != queue) {
        list_node_t *next_node = node->next;
        // printl("wakeup: %lx\n", node);
        // 这里do_unblock的约定由wakeup外部代为履行
        do_unblock(node); // 唤醒任务
        node = next_node; // 继续检查下一个节点
    }
}

// 回收一个进程的所有锁
void free_lock(pid_t pid){
    for (int i = 0; i < LOCK_NUM; i++) {
        if (mlocks[i].pid == pid) {
            do_mutex_lock_release(i);
        }
    }
}
// 释放屏障
void free_barrier(){
    for (int idx = 0; idx < BARRIER_NUM; idx++) {
        spin_lock_acquire(&barriers[idx].lock);
        if (barriers[idx].arrived > 0) {
            // 确认当前进程是否在屏障的阻塞队列中
            if (is_in_queue(&current_running->list, &barriers[idx].block_queue)) {
                do_unblock(&current_running->list); // 踢出阻塞队列
                barriers[idx].arrived--;
            }
        }
        spin_lock_release(&barriers[idx].lock);
    }
}

void free_mbox()
{
    for (int idx = 0; idx < MBOX_NUM; idx++) {
        spin_lock_acquire(&mailboxes[idx].lock);
        if (mailboxes[idx].user_count > 0) {
            // 确认当前进程是否在mailbox的阻塞队列中
            if (is_in_queue(&current_running->list, &mailboxes[idx].send_queue) ||
                is_in_queue(&current_running->list, &mailboxes[idx].recv_queue))
            {
                do_unblock(&current_running->list);
                mailboxes[idx].user_count--;
            }
        }
        spin_lock_release(&mailboxes[idx].lock);
    }
}

// 强制一个非当前进程退出：暂时让current_running 指向该进程
void force_exit(pcb_t *pcb_ptr){
    pcb_t *current_running_back = current_running;
    current_running = pcb_ptr;
    printl("force_exit: pid %d exiting ...\n", current_running->pid);
    free_lock(current_running->pid);
    free_barrier();
    free_mbox();

    if (current_running->thread_group_id == 0) {
        extern void free_all_pages(uintptr_t pgdir_kva);
        free_all_pages(current_running->pgdir_kva);
    }
    // TODO: 记录线程栈并释放

    current_running->status = TASK_EXITED;
    current_running->killed = 1;
    current_running->thread_group_id = 0; //清空线程标记
    current_running->mask = MASK_ALL; // 恢复默认mask
    wakeup(&current_running->wait_list);
    current_running = current_running_back; // 恢复current_running
}

void do_exit(){
    printl("do_exit: pid %d exiting ...\n", current_running->pid);
    // TODO: 回收资源
    free_lock(current_running->pid);
    free_barrier();
    free_mbox();

    // 主进程退出代表整个程序结束
    printl("  current_running->thread_group_id: %d\n",current_running->thread_group_id);
    if (current_running->thread_group_id == 0) {
        // 这是一个主进程，拥有页表的所有权
        printl("  free_all_pages\n");
        free_all_pages(current_running->pgdir_kva);
    }
    // TODO: 记录线程栈并释放

    current_running->status = TASK_EXITED;
    current_running->killed = 1;
    current_running->thread_group_id = 0; //清空线程标记
    current_running->mask = MASK_ALL; // 恢复默认mask
    // wakeup约定：调用者保护唤醒队列
    wakeup(&current_running->wait_list);
    do_scheduler(); // 回收cpu
}

//// 1.改变状态 2.从所在队列删除 3.释放锁 4.唤醒等待队列
//// do_kill 销毁 一个进程时，它必须获取所有可能操作该 PCB 状态的锁
// do_kill 只负责设置killed标志位
// status的修改和唤醒睡眠进程交给do_scheduler/release/wait/recv/send调用do_exit
// 从就绪队列/睡眠队列中删除交给do_scheduler：调度为current_running后回来发现自己被killed，也就自动从就绪队列中出来了
// 从资源的等待队列中删除交给同步原语release/wait/recv/send操作，因为抢不到锁导致睡眠醒来后，(此时已经从等待队列中出来了)发现自己被killed，调用exit()
int do_kill(pid_t pid){
    printl("killing: pid %d\n", pid);
    if (pid == 0) {
        printl("do_kill: cannot kill kernel process\n");
        return 0; // 失败
    }
    if (pid == current_running->pid) {
        printl("do_kill: cannot kill itself\n");
        return 0;
        // printl("do_kill: kill self, exit\n");
        // do_exit();
        // return 1;
    }
    pcb_t *pcb_ptr = get_pcb(pid);


    if (pcb_ptr == NULL || pid < 0 || pid > process_id) {
        printl("do_kill: cannot find pid %d process\n", pid);
        return 0; // 失败
    }

    // ! 缺陷：不知道该进程处于哪个队列中，如果存在于锁的等待队列可能无法删除
    pcb_ptr->killed = 1;
    printl("do_kill: kill pid %d successfully\n", pid);
    return 1; // 成功
}

int do_waitpid(pid_t pid){
    printl("pid %d is waiting for pid %d...\n\n", current_running->pid, pid);
    pcb_t *pcb_ptr = get_pcb(pid);

    if (pcb_ptr != NULL) {
        // 原子性地 释放锁，阻塞入发送队列
        disable_preempt();
        // do_block约定：需要调用者保护阻塞队列锁
    
        do_block(&current_running->list, &pcb_ptr->wait_list);
        enable_preempt();
        do_scheduler(); // 主动让出cpu
        return pid;
    }else {
        return 0;
    }

}

void p_b(int num){
    for (int i = 0; i < num; i++) {
        printk(" ");
    }
}
// 十进制数的位数
int i_len(int num){
    if (num == 0) return 1;
    int len = 0;
    while (num) {
        len++;
        num /= 10;
    }
    return len;
}

void do_process_show(){
                                                            // 需要打印的字符数  + 一个空格
    printk("No. "); p_b(1);                        // 4
    printk("PID ");                                     // 3
    printk("name "); p_b(MAX_NAME_LEN - 4);        // 15
    printk("STATUS "); p_b(MAX_STATUS_LEN-6);      // 7
    printk("mask "); // 不够长                          // 4
    printk("Running_Core\n");  

    int valid_idx = 0;
    for (int i = 0; i < TASK_MAXNUM; i++) {
        if (pcb[i].status != TASK_EXITED) {
            // 序号
            printk("[%d] ", valid_idx);
            p_b(4 - 2 - i_len(valid_idx++));
            // pid
            printk("%d ", pcb[i].pid);
            p_b(3 - i_len(pcb[i].pid));
            // name
            printk("%s ", pcb[i].name);
            p_b(MAX_NAME_LEN - strlen(pcb[i].name));
            // STATUS
            char* str = STATUS2STR(pcb[i].killed, pcb[i].status);
            printk("%s ", str);
            p_b(MAX_STATUS_LEN - strlen(str));
            // mask
            printk("0x%d ", pcb[i].mask);
            p_b(4 - 2 - i_len(pcb[i].mask));
            // Running_Core
            if (pcb[i].status == TASK_RUNNING){
                printk(" Core %d\n", pcb[i].cpuid);
            }
            else{
                printk("\n");
            }
        }
    }



    // for (int i = 0; i < TASK_MAXNUM; i++) {
    //     if (pcb[i].status != TASK_EXITED) {
    //         printk("[%d] \tname : %s \n", valid_idx++, pcb[i].name);
    //         printk("     PID : %d  STATUS : %s\tmask : 0x%d", pcb[i].pid, 
    //         STATUS2STR(pcb[i].killed, pcb[i].status), pcb[i].mask);
    //         if (pcb[i].status == TASK_RUNNING){
    //             printk(" Running on core %d\n", pcb[i].cpuid);
    //         }else{
    //             printk("\n");
    //         }
    //     }
    // }


}

pid_t do_getpid(){
    return current_running->pid;
}

void do_taskset(int mask, char *name, int pid, int argc, char **argv){
    // taskset -p <mask> <pid>
    // sys_taskset(mask, NULL, pid, argc, argv); 
    // taskset <mask> <task_name>
    // sys_taskset(mask, argv[1], 0, exec_argc, exec_argv); 

    // 先检验mask
    if (mask <= 0) {
        printl("[ERROR] do_taskset: Invalid mask (0x%lx)\n", mask);
        return;
    }
    if ((mask & ((1 << NR_CPUS) - 1)) == 0) {
        printl("[ERROR] do_taskset: Mask (0x%lx) contains no valid CPUs, cpu num: %d\n", mask, NR_CPUS);
    }

    pid_t tar_pid = pid; // 默认使用传入的pid查找进程
    // 约定：-p 时一定不会传递name，有name就是要调用exec
    if (name != NULL) {
        tar_pid = do_exec(name, argc, argv); // 有name时使用exec返回的pid
    }

    pcb_t *pcb_ptr = get_pcb(tar_pid);

    if (pcb_ptr == NULL) {
        printl("[Warning] do_taskset: Process with PID %d not found.\n", pid);
        return ;
    }

    pcb_ptr->mask = mask; // 更新cpu掩码
    // pcb_ptr->mask_set = 1; // 更新cpu掩码标志
    // 若该进程正在运行且不在掩码允许的cpu上，需要立即调度修正
    if (pcb_ptr->status == TASK_RUNNING) {
        int cpuid = pcb_ptr->cpuid;
        if (!(mask & (1 << cpuid))) {
            printl("[Warning] do_taskset: PID %d forced to reschedule from Core %d.\n", pid, cpuid);
            do_scheduler();
        }
    }
}

// start_routine: 线程函数入口  arg：参数指针
// 成功：返回线程的pid；失败：返回0
// 如果后续还需要do_thread_join操作，需要一个数组记录线程pid
pid_t do_thread_create(ptr_t start_routine, ptr_t arg, int arg_bytes)
{
    printl("thread_creating ...\n");
 
    // 查找空闲 PCB 槽位
    int idx = 0;
    for (; idx < TASK_MAXNUM; idx++) {
        if (pcb[idx].status == TASK_EXITED) break;
    }
    if (idx == TASK_MAXNUM) {
        printl("[ERROR] No more available pcb space!\n\n");
        return 0;
    }

    // 初始化新 PCB (线程)
    pcb[idx].pid = process_id++;
    pcb[idx].status = TASK_READY; // 先设置状态，防止被分配给别的进程
    pcb[idx].thread_group_id = current_running->pid; //  设置线程组 ID (共享父进程的 PID)
    pcb[idx].killed = 0; // 复活
    pcb[idx].mask = current_running->mask; // 继承父进程的mask
    pcb[idx].dir_inode = current_running->dir_inode;

    // 为线程分配并映射独立的栈空间
    // 策略：使用 pid 作为距离用户栈底的偏移，避免冲突
    // 注意：这假设栈不会溢出覆盖到其他线程。
    uintptr_t thread_stack_base_uva = USER_STACK_ADDR - PAGE_SIZE - (pcb[idx].pid * PAGE_SIZE);
    
    // 分配物理页并建立映射 (使用父进程的页表)
    uintptr_t thread_stack_base_kva = alloc_page_helper(thread_stack_base_uva - PAGE_SIZE, current_running->pgdir_kva, pcb[idx].pid);
    // 注意：alloc_page_helper 映射的是 Page Base，但栈顶是 Base + Size
    
    pcb[idx].pgdir_kva = current_running->pgdir_kva; // 共享页表

    // 制作线程上下文 (sepc = start_routine, a0 = arg)
    init_thread_stack(
        pcb[idx].kernel_stack_base + PAGE_SIZE,
        thread_stack_base_kva + PAGE_SIZE, 
        thread_stack_base_uva + PAGE_SIZE,
        start_routine, 
        &pcb[idx],
        arg, // 传递给 start_routine 的参数，该参数已经在用户内存中？？只需要将a0赋值为arg即可
        arg_bytes // 参数字节数
    );
    init_queue(&pcb[idx].list);
    init_queue(&pcb[idx].wait_list);
    printl("     create thread %d of process %d...\n", pcb[idx].pid, pcb[idx].thread_group_id);
    // strcpy(pcb[idx].name, current_running->name);
    strcpy(pcb[idx].name, "_thread");
    // strcat(pcb[idx].name, "_thread");

    // 加入就绪队列
    enqueue(&pcb[idx].list, &ready_queue);

    return pcb[idx].pid;
}


// 让调用进程（父线程）阻塞，直到目标线程(由pid查找)完成执行并退出
// 失败返回0；成功等待线程结束后醒来返回线程pid；
pid_t do_thread_join(pid_t pid)
{
    // 查找目标线程的 PCB
    pcb_t *pcb_ptr = get_pcb(pid);

    // 确保目标线程还未退出 (TASK_EXITED 或 killed)
    if (pcb_ptr != NULL) {
        // 检查目标线程是否在当前线程组内
        if (pcb_ptr->thread_group_id != current_running->pid) {
            // 目标线程不属于当前进程，可能是传入的pid有误
            printl("[ERROR] do_thread_join: pid %d is not a thread of pid %d!\n", 
                pid, current_running->pid);
            return 0; // 失败
        }
        // 原子性地 释放锁，阻塞入发送队列。防死锁
        disable_preempt();
        // 将当前进程阻塞到目标线程的 wait_list
        do_block(&current_running->list, &pcb_ptr->wait_list); 
        enable_preempt();
        do_scheduler(); // 主动让出cpu
        //  醒来后，线程已退出，返回线程 PID
        return pid;
    } else {
        // 目标线程不存在或已退出或被杀死
        return 0; // 失败
    }
}