#include <os/lock.h>
#include <os/list.h>
#include <os/sched.h>
#include <os/proc.h>
#include <printk.h>
#include <os/irq.h>

condition_t conditions[CONDITION_NUM];

void init_conditions(void){
    for (int i = 0; i < CONDITION_NUM; i++) {
        conditions[i].key = UNUSED;
        init_queue(&conditions[i].block_queue);
        spin_lock_init(&conditions[i].lock);
    }
}
int do_condition_init(int key){
    if (key < 0) {
        printl("[Warning] do_condition_init: invalid key %d, must >= 0\n", key);
        return -1; 
    }

    // 资源分配
    
    for (int i = 0; i < CONDITION_NUM; i++) {
        if (conditions[i].key == key) {
            
            return i;
        }
    }
    for (int i = 0; i < CONDITION_NUM; i++) {
        if (conditions[i].key == UNUSED) {
            conditions[i].key = key;
            
            return i;
        }
    }
    
    return -1; 
}
void do_condition_wait(int cond_idx, int mutex_idx){
    // 检查索引有效性
    if (cond_idx < 0 || cond_idx >= CONDITION_NUM) {
        printl("[Warning] do_condition_wait: invalid cond_idx %d\n", cond_idx);
        return; 
    }
    if (mutex_idx < 0 || mutex_idx >= LOCK_NUM) {
        printl("[Warning] do_condition_wait: invalid mutex_idx %d\n", mutex_idx);
        return; 
    }

    // 阻塞，释放锁，调度
    spin_lock_acquire(&conditions[cond_idx].lock);

    disable_preempt();
    do_block(&current_running->list, &conditions[cond_idx].block_queue);
    do_mutex_lock_release(mutex_idx);
    // 若在这里被抢占，该进程P0释放了资源锁但持有条件变量锁，且已被阻塞，无法释放条件变量锁
    // P1可以获得资源锁，到最后V操作时由于无法获取条件变量锁导致死锁
    spin_lock_release(&conditions[cond_idx].lock);
    enable_preempt();
    do_scheduler();
    // 醒来后检查自己是否被杀死
    if (current_running->killed) {
        do_exit(); 
    }  
    // 进程被唤醒，重新获取锁
    do_mutex_lock_acquire(mutex_idx);

}
void do_condition_signal(int cond_idx){
    // 检查索引有效性
    if (cond_idx < 0 || cond_idx >= CONDITION_NUM) {
        printl("[Warning] do_condition_signal: invalid cond_idx %d\n", cond_idx);
        return; 
    }

    spin_lock_acquire(&conditions[cond_idx].lock);
    if (!is_empty_queue(&conditions[cond_idx].block_queue)) {
        list_node_t *node = dequeue(&conditions[cond_idx].block_queue);
        do_unblock(node);
        // P1被唤醒，若在这里被抢占，会等待条件变量的锁，最后还能回来P0释放锁，也不会发生什么
    }
    spin_lock_release(&conditions[cond_idx].lock);
}
void do_condition_broadcast(int cond_idx){
    // 检查索引有效性
    if (cond_idx < 0 || cond_idx >= CONDITION_NUM) {
        printl("[Warning] do_condition_broadcast: invalid cond_idx %d\n", cond_idx);
        return; 
    }
    spin_lock_acquire(&conditions[cond_idx].lock);
    wakeup(&conditions[cond_idx].block_queue);
    spin_lock_release(&conditions[cond_idx].lock);
}
void do_condition_destroy(int cond_idx){
    // 检查索引有效性
    if (cond_idx < 0 || cond_idx >= CONDITION_NUM) {
        printl("[Warning] do_condition_destroy: invalid cond_idx %d\n", cond_idx);
        return; 
    }
    // 资源销毁：全局锁
    
    if (!is_empty_queue(&conditions[cond_idx].block_queue)) {
        printl("[Warning] Condition %d destroyed while tasks are blocked.\n", cond_idx);
        wakeup(&conditions[cond_idx].block_queue);
    }
    conditions[cond_idx].key = UNUSED;
    
}
