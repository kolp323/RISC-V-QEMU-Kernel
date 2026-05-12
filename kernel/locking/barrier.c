#include <os/lock.h>
#include <os/list.h>
#include <os/sched.h>
#include <os/proc.h>
#include <printk.h>
#include <os/irq.h>

barrier_t barriers[BARRIER_NUM];

void init_barriers(void){
    for (int i = 0; i < BARRIER_NUM; i++) {
        barriers[i].key = UNUSED;
        init_queue(&barriers[i].block_queue);
        spin_lock_init(&barriers[i].lock);
        barriers[i].goal = 0;
        barriers[i].arrived = 0;
    }
}

int do_barrier_init(int key, int goal){
    if (key < 0) {
        printl("[Warning] do_barrier_init: invalid key %d, must >= 0\n", key);
        return -1; 
    }
    if (goal <= 0) {
        printl("[Warning] do_barrier_init: invalid goal %d, must > 0\n", goal);
        return -1; 
    }

    // 分配新资源：需要获取全局锁
    for (int i = 0; i < BARRIER_NUM; i++) {
        if (barriers[i].key == key) {
            barriers[i].goal = goal; // 允许重设目标
            
            return i;
        }
    }

    for (int i = 0; i < BARRIER_NUM; i++) {
        if (barriers[i].key == UNUSED) {
            barriers[i].goal = goal;
            barriers[i].key = key;
            
            return i;
        }
    }

    
    return -1; // 初始化失败
}
void do_barrier_wait(int bar_idx){
    // 检查索引有效性
    if (bar_idx < 0 || bar_idx >= BARRIER_NUM) {
        printl("[Warning] do_barrier_wait: invalid bar_idx %d\n", bar_idx);
        return; 
    }

    spin_lock_acquire(&barriers[bar_idx].lock);
    barriers[bar_idx].arrived++;
    if (barriers[bar_idx].arrived >= barriers[bar_idx].goal) {
        // 达到目标，唤醒所有等待进程
        barriers[bar_idx].arrived = 0; // 重置计数，为下一轮做准备
        wakeup(&barriers[bar_idx].block_queue);
        // 部分唤醒时，发生中断，由于上一轮的最后一个进程还没有释放锁，
        // 导致进入下一轮的所有进程无法在进入do_barrier_wait后执行后续操作
        // 安全 √
        spin_lock_release(&barriers[bar_idx].lock);
    }else {
        // 未达到目标，阻塞当前进程并调度
        // TODO：原子性地实现释放锁并阻塞
        // 单核情况：
        // 如果先阻塞，可能在释放锁前被中断，此时另一个进程到达屏障并尝试获取锁，它会一直自旋等待
        // 如果先释放锁，在阻塞前被中断，此时最后一个进程到达屏障，唤醒所有进程，但该进程因为没有被阻塞所以错过唤醒，回到该进程后继续完成未完成的阻塞操作，导致该进程留在了上一层屏障前
        // *解决方法*：关中断
        disable_preempt();
        // 多核情况：
        // C0先释放锁，C1的最后一个进程获得锁，执行唤醒，由于C0未执行阻塞，导致错过唤醒
        // C0先阻塞（它不会被本核中断），它持有锁，其他核上的进程无法获取锁，直到它释放锁。安全√
        do_block(&current_running->list, &barriers[bar_idx].block_queue);
        spin_lock_release(&barriers[bar_idx].lock);
        enable_preempt();
        do_scheduler();
        // 醒来后检查自己是否被杀死
        if (current_running->killed) {
            do_exit(); 
        }  
    }

}
void do_barrier_destroy(int bar_idx){
    if (bar_idx < 0 || bar_idx >= BARRIER_NUM) {
        printl("[Warning] do_barrier_destroy: invalid bar_idx %d\n", bar_idx);
        return; 
    }

    // 销毁资源：需要分配全局锁

    // 如果 block_queue 不为空，说明有进程死锁或逻辑错误。
    if (!is_empty_queue(&barriers[bar_idx].block_queue)) {
        printl("[Warning] do_barrier_destroy: Barrier %d destroyed while tasks are blocked.\n", bar_idx);
        wakeup(&barriers[bar_idx].block_queue);
    }
    barriers[bar_idx].key = UNUSED;
    barriers[bar_idx].goal = 0;
    barriers[bar_idx].arrived = 0;
    init_queue(&barriers[bar_idx].block_queue);
    
}
