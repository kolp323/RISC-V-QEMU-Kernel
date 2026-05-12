#include <os/list.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/proc.h>
#include <os/task.h>
#include <os/time.h>
#include <os/mm.h>
#include <screen.h>
#include <printk.h>
// #include <assert.h>
#include <os/workload.h>
#include <os/irq.h>
#include <os/smp.h>
#include <os/fs.h>

// 将pcb状态字段的保护职责转移到队列锁上:修改状态和队列操作必须一起原子化
// 进入wait_list时：状态字段修改由pcb_array_lock保护
// 进入ready_queue时：状态字段修改由ready_queue_lock保护
// 进入sleep_queue时：状态字段修改由sleep_queue_lock保护

pcb_t pcb[TASK_MAXNUM];
pcb_t kpcb[NR_CPUS];

LIST_HEAD(ready_queue);
LIST_HEAD(sleep_queue);

/* global process id */
pid_t process_id = 1;


void init_pcb(void)
{
    printl("init locks...");
    // 初始化内核IDLE进程PCB
    printl("initialize kernel PCB...");
    kpcb[0].pid = 0;
    kpcb[0].kernel_sp = kpcb[0].user_sp = KERNEL_STACK_0; // 0x50500000 - 288只允许使用上下文空间以下的区域
    kpcb[0].full_context = KERNEL_STACK_0; // 指向完整上下文空间
    init_queue(&kpcb[0].wait_list);
    kpcb[0].status = TASK_RUNNING;
    kpcb[0].cpuid = 0; 
    // kpcb[0].mask_set = 0; 
    kpcb[0].mask = MASK_ALL; // 便于给用户进程设置默认mask
    kpcb[0].pgdir_kva = pa2kva(PGDIR_PA);   
    kpcb[0].dir_inode = ROOT_INODE_NUM; // 初始化为根目录 
    printl("pid: %d, kernel_sp: 0x%lx\n\n", pcb[0].pid, pcb[0].kernel_sp);



    printl("init PCBs...");
    for (int i = 0; i < TASK_MAXNUM; i++){
        pcb[i].status = TASK_EXITED;
        pcb[i].killed = 1; // 豆沙了 (*v*)
        pcb[i].thread_group_id = 0; // 线程组id初始化为0，表示是一个独立进程
        pcb[i].mask = MASK_ALL; // 默认mask
        // pcb[i].mask_set = 0; // 未设置mask
        pcb[i].dir_inode = ROOT_INODE_NUM; // 初始化为根目录 
        // 初始化等待队列
        init_queue(&pcb[i].wait_list);
        pcb[i].kernel_sp = pcb[i].kernel_stack_base = allocPage(KERNEL_PAGE_NUM); // 栈底地址
    }

    /* TODO: [p2-task1] remember to initialize 'current_running' */
    printl("Initializing current_running...\n");
    current_running = &kpcb[0];
}

// 辅助函数：判断任务是否允许在当前核心运行
int if_mask_match(list_node_t *node){
    uint64_t cpuid = get_current_cpu_id();
    int current_cpu_mask = 1 << cpuid;
    pcb_t *pcb_ptr = CONTAINER_OF(node, pcb_t, list);
    // return pcb_ptr->mask_set ? (pcb_ptr->mask & current_cpu_mask) : 1; // 未被设置过mask可以随便运行所有核上
    return pcb_ptr->mask & current_cpu_mask; 
}

// TODO:???清理僵尸进程
void clean_zombie(){
    for (int i = 0; i < TASK_MAXNUM; i++) {
        if (pcb[i].status == TASK_BLOCKED && pcb[i].killed == 1) {
            force_exit(&pcb[i]);
        }
    }
}

void do_scheduler(void)
{
    uint64_t cpuid = get_current_cpu_id();

    // 内部有sleep_queue的锁处理
    check_sleeping();
    clean_zombie();
    /************************************************************/
    // TODO: [p5-task3] Check send/recv queue to unblock PCBs
    /************************************************************/

    // TODO: [p2-task1] Modify the current_running pointer.
    // TODO: [p2-task1] switch_to current_running
    pcb_t *prev = current_running;
    int set_ready = 0;
    // 非IDLE进程加入就绪队列
    
    if (prev->status == TASK_RUNNING && prev->pid != 0) { 
        set_ready = 1; // !prev和next的状态要一起改变，否则ps很难展示正在运行的任务
        enqueue(&prev->list, &ready_queue);
    }

    // TODO: 找到一个能在当前核心上运行的任务
    // list_node_t *next_node = dequeue(&ready_queue);
    list_node_t *next_node = visit(&ready_queue, if_mask_match);
      // 就绪队列操作完毕
    
    pcb_t *next;
    
    if (next_node == NULL) { // 队空或找不到能在当前cpu运行的进程，进入IDLE进程
        next = &kpcb[cpuid];
    }else{ 
        next = CONTAINER_OF(next_node, pcb_t, list);
        next->cpuid = cpuid;
    } 
    if(set_ready) prev->status = TASK_READY;
    current_running = next;
    current_running->status = TASK_RUNNING;
    
    // [p4-task1] 切换页表
    uintptr_t pgdir_pa = kva2pa(current_running->pgdir_kva);
    set_satp(
        SATP_MODE_SV39, 
        current_running->pid, 
        pgdir_pa >> NORMAL_PAGE_SHIFT
    );
    local_flush_tlb_all();
    
    // 执行上下文切换
    switch_to(prev, current_running);

    // 死锁被kill的僵尸进程无法被清理
    if (current_running->killed) {
        do_exit(); // 自杀，释放所有资源
    }
}

void do_sleep(uint32_t sleep_time)
{
    // TODO: [p2-task3] sleep(seconds)
    // NOTE: you can assume: 1 second = 1 `timebase` ticks
    // 1. block the current_running
    // 2. set the wake up time for the blocked task
    // 3. reschedule because the current_running is blocked.
    current_running->wakeup_time = get_timer() + sleep_time;  // 设置唤醒时间
    
    do_block(&current_running->list, &sleep_queue); // 阻塞当前任务
    
    do_scheduler(); // 重新调度
}

// 阻塞：需要调用者保护传入的阻塞队列queue，一般只会阻塞current_running。
void do_block(list_node_t *pcb_node, list_head *queue)
{
    // TODO: [p2-task2] block the pcb task into the block queue
    pcb_t *pcb_ptr = CONTAINER_OF(pcb_node, pcb_t, list);
    // 不需要删除操作：任务在 TASK_RUNNING 时，不可能在 ready_queue 中。
    // del_node(pcb_node); // 先从当前就绪队列删除
    if (pcb_ptr->killed != 1) { // 就别鞭尸了QAQ
        enqueue(pcb_node, queue); // 加入阻塞队列
        pcb_ptr->status = TASK_BLOCKED; // 设置阻塞态
    }
    // spin_lock_release(&pcb_array_lock);
}

// 从阻塞队列中唤醒任务：外部调用者应该保护原有队列，内部锁保护就绪队列
void do_unblock(list_node_t *pcb_node)
{
    // TODO: [p2-task2] unblock the `pcb` from the block queue
    del_node(pcb_node);

    pcb_t *pcb_ptr = CONTAINER_OF(pcb_node, pcb_t, list);
    
    pcb_ptr->status = TASK_READY;
    enqueue(pcb_node, &ready_queue);
    
}


// 到达checkpoint时next_target设为终点，done_cnt清零
// 到达终点时next_target设为checkpoint，done_cnt清零
void do_set_sche_workload(int remain_length){
    // disable_preempt();
    printl("entering do_set_sche_workload()...\n");
    int cur_len = LENGTH - remain_length;
    // 到达目标判断和处理
    if (cur_len >= current_running->next_target || current_running->progress >= 100){
        done_cnt++;
        // del_node(&current_running->list); // 先从当前就绪队列删除（doblock中已有）
        
        do_block(&current_running->list, &sleep_queue); // 阻塞当前任务
        
    }
    // 全部到达目标点后重设目标和状态，全部唤醒
    if (done_cnt >= FLY_NUM) {
        done_cnt = 0;
        stage = (stage == CHECKPOINT) ? END : CHECKPOINT;
        reset_target_wakeup((stage == CHECKPOINT) ? check_point : end_point);
        // enable_preempt();
        return;
    }

    // 没有全到，更新进度
    current_running->progress = (stage == CHECKPOINT) ? 
        cur_len*100/current_running->next_target : 
        (cur_len - check_point[current_running->pid - 1])*100/check_to_end[current_running->pid - 1];
    printl("fly%d: target(%d) progress(%d)\n", current_running->pid, current_running->next_target, current_running->progress);
    // enable_preempt();
}