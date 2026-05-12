#include <os/lock.h>
#include <os/sched.h>
#include <os/proc.h>
#include <os/list.h>
#include <atomic.h>
#include <os/irq.h>
#include <printk.h>

mutex_lock_t mlocks[LOCK_NUM];

void init_locks(void)
{
    /* TODO: [p2-task2] initialize mlocks */
    for (int i = 0; i < LOCK_NUM; i++) {
        init_queue(&mlocks[i].block_queue);
        spin_lock_init(&mlocks[i].lock); // 初始化自旋锁
        mlocks[i].status = UNLOCKED;
        mlocks[i].key = UNUSED; // -1:表示未使用
        mlocks[i].pid = 0;
    }

}

void spin_lock_init(spin_lock_t *lock)
{
    /* TODO: [p2-task2] initialize spin lock */
    atomic_swap(UNLOCKED, (ptr_t)(&lock->status));
}

int spin_lock_try_acquire(spin_lock_t *lock)
{
    /* TODO: [p2-task2] try to acquire spin lock */
    int old_status = atomic_swap(LOCKED, (ptr_t)(&lock->status));
    return (old_status == UNLOCKED); // 原来的状态是UNLOCKED，获取成功
    // return 0;
}

void spin_lock_acquire(spin_lock_t *lock)
{
    /* TODO: [p2-task2] acquire spin lock */
    while (!spin_lock_try_acquire(lock)) ; // 自旋等待
}

void spin_lock_release(spin_lock_t *lock)
{
    /* TODO: [p2-task2] release spin lock */
    atomic_swap(UNLOCKED, (ptr_t)(&lock->status));
}

int do_mutex_lock_init(int key)
{
    if (key < 0) {
        printl("[Warning] do_mutex_lock_init: invalid key %d, must >= 0\n", key);
        return -1; 
    }
    // 保护 mlocks 数组的分配
    

    /* TODO: [p2-task2] initialize mutex lock */
    // 寻找key值匹配的锁
    for (int i = 0; i < LOCK_NUM; i++) {
        if (key == mlocks[i].key){
            
            return i;
        }
    }
    // 如果是新的key，返回一个未使用的锁
    for (int i = 0; i < LOCK_NUM; i++) {
        if (mlocks[i].key == UNUSED){ 
            mlocks[i].key = key;
            
            return i;
        }
    }
    
    return -1;
}

void do_mutex_lock_acquire(int mlock_idx)
{
    if (mlock_idx < 0 || mlock_idx >= LOCK_NUM) {
        printl("[Warning] do_mutex_lock_acquire: invalid mlock_idx %d\n", mlock_idx);
        return; 
    }

    spin_lock_acquire(&mlocks[mlock_idx].lock); // 保护锁状态
    /* TODO: [p2-task2] acquire mutex lock */
    if (mlocks[mlock_idx].status == LOCKED) {
        disable_preempt();
        do_block(&current_running->list, &mlocks[mlock_idx].block_queue);
        // 若阻塞后被抢占，mutex自旋锁还被P0持有，但P0被阻塞无法释放，当P1试图释放mutex时，会因为自旋锁而无限等待
        spin_lock_release(&mlocks[mlock_idx].lock);
        enable_preempt();
        do_scheduler();
        // 醒来后检查自己是否被杀死
        if (current_running->killed) {
            do_exit(); 
        }    

        return;
    }
    mlocks[mlock_idx].status = LOCKED;
    mlocks[mlock_idx].pid = current_running->pid;

    spin_lock_release(&mlocks[mlock_idx].lock);
    return;
}

void do_mutex_lock_release(int mlock_idx)
{
    if (mlock_idx < 0 || mlock_idx >= LOCK_NUM) {
        printl("[Warning] do_mutex_lock_release: invalid mlock_idx %d\n", mlock_idx);
        return; 
    }
    spin_lock_acquire(&mlocks[mlock_idx].lock);
    /* TODO: [p2-task2] release mutex lock */
    // 只有所有者进程可以释放自己的锁
    if (current_running->pid != mlocks[mlock_idx].pid) {
        return;
    }
    // 若阻塞队列非空，唤醒队头，锁仍然保持LOCKED状态
    if (!is_empty_queue(&mlocks[mlock_idx].block_queue)) {
        list_node_t *next_node = dequeue(&mlocks[mlock_idx].block_queue);
        do_unblock(next_node);
        pcb_t *next = CONTAINER_OF(next_node, pcb_t, list);
        mlocks[mlock_idx].pid = next->pid; // 更改所有者进程
    }else{ // 状态队列空，清理锁状态
        mlocks[mlock_idx].status = UNLOCKED;
        mlocks[mlock_idx].pid = 0;
        mlocks[mlock_idx].key = UNUSED;
    }
    spin_lock_release(&mlocks[mlock_idx].lock);
    return;
}
