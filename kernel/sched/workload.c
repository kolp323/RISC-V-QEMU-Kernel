#include <os/time.h>
#include <os/sched.h>
#include <printk.h>
#include <os/kernel.h>
#include <os/workload.h>
#include <os/irq.h>
const int check_point[FLY_NUM] = {10, 20, 30, 40, 50};
const int end_point[FLY_NUM] = {LENGTH, LENGTH, LENGTH, LENGTH, LENGTH};
const int check_to_end[FLY_NUM] = {LENGTH - 10, LENGTH - 20, LENGTH - 30, LENGTH - 40, LENGTH - 50};

int stage = CHECKPOINT; // 当前阶段
int done_cnt = 0; // 到达目标点的飞机数

void init_fly(){
    for (int i = 1; i <= FLY_NUM; i++){
        pcb[i].next_target = check_point[i-1];
        pcb[i].progress = 0;
    }
}

void reset_target_wakeup(const int* points){
    
    for (int i = 1; i <= FLY_NUM; i++) {
        pcb[i].next_target = points[i-1];
        pcb[i].progress = 0;
        do_unblock(&pcb[i].list);
    }
    
}


void do_workload_scheduler(){
    disable_preempt();
    printl("entering do_workload_scheduler()...\n");
    pcb_t *prev = current_running;
    if (prev->status == TASK_RUNNING && prev->pid != 0) { // 进程0不能被调度
        prev->status = TASK_READY;
        enqueue(&prev->list, &ready_queue); // 加入就绪队列
    }
    if (is_empty_queue(&ready_queue)) return; // 继续运行当前任务 (pid 0) 或等待中断

    // 寻找progress最小的任务
    list_node_t *next_node = ready_queue.next;
    list_node_t *min_node = ready_queue.next;
    int min_progress = __INT32_MAX__;
    while (next_node != &ready_queue) {
        pcb_t *pcb_ptr = CONTAINER_OF(next_node, pcb_t, list);
        if (pcb_ptr->progress <= min_progress) {
            min_progress = pcb_ptr->progress;
            min_node = next_node;
        }
        next_node = next_node->next;
    }

    // 切换到progress最小的任务
    del_node(min_node);
    pcb_t *next = CONTAINER_OF(min_node, pcb_t, list);
    current_running = next;
    current_running->status = TASK_RUNNING;
    printl(">>[SCHED] Switch from PID %d  to PID %d\n", prev->pid, current_running->pid);
    printl("\t kernel_sp: 0x%lx to 0x%lx\n", prev->kernel_sp, current_running->kernel_sp);
    
    // int progress_lim = (current_running->progress >= 75) ? 106 : 110;
    int progress_lim = 116;
    int allocated_time = 
        TIMER_INTERVAL * (progress_lim - current_running->progress) / 100;
    printl("\tallocted time: %d\n", allocated_time);
    bios_set_timer(get_ticks() + allocated_time); // 设置下次中断时间

    enable_preempt();
    // 执行上下文切换
    switch_to(prev, current_running);
}