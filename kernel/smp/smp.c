#include <atomic.h>
#include <os/sched.h>
#include <os/smp.h>
#include <os/lock.h>
#include <os/kernel.h>
#include <os/irq.h>
#include <os/mm.h>
#include <os/fs.h>
#include <printk.h>
#include <pgtable.h>

spin_lock_t kernel_lock;
// 递归锁：记录持有锁的 CPU 和深度
volatile int bkl_owner = -1; // -1 表示无主
volatile int bkl_depth = 0;

void smp_init()
{
    /* TODO: P3-TASK3 multicore*/
    // printl("init smp...\n");
    int i = get_current_cpu_id();
    kpcb[i].pid = 0; 
    kpcb[i].kernel_sp = kpcb[i].user_sp = KERNEL_STACK_1;
    kpcb[i].full_context = KERNEL_STACK_1;
    init_queue(&kpcb[i].wait_list);
    kpcb[i].status = TASK_RUNNING; 
    kpcb[i].mask = MASK_ALL; // 便于给用户进程设置默认mask
    kpcb[i].cpuid = i;   // 核心 i
    kpcb[i].pgdir_kva = pa2kva(PGDIR_PA);  
    kpcb[i].dir_inode = ROOT_INODE_NUM; // 初始化为根目录 
    current_running = &kpcb[i]; // 从核的IDLE进程
    init_exception();   // Init interrupt (^_^)
}


void wakeup_other_hart()
{
    /* TODO: P3-TASK3 multicore*/
    unsigned long hart_mask = MASK_ONE;
    send_ipi(&hart_mask);
    uint64_t cpu_id = get_current_cpu_id();
}

void lock_kernel()
{
    /* TODO: P3-TASK3 multicore*/
    int cpuid = get_current_cpu_id();
    // 检查当前 CPU 是否已经持有锁
    if (bkl_owner == cpuid) {
        bkl_depth++; // 是，则增加递归深度
    } else {
        spin_lock_acquire(&kernel_lock); // 否，则竞争锁
        bkl_owner = cpuid; // 记录拥有者
        bkl_depth = 1;     // 初始化递归深度
    }
}

void unlock_kernel()
{
    /* TODO: P3-TASK3 multicore*/
    int cpuid = get_current_cpu_id();
    // 只有锁的拥有者才能解锁 (安全性检查)
    if (bkl_owner == cpuid) {
        bkl_depth--; // 减少递归深度
        // 当递归深度归零时，才真正释放锁
        if (bkl_depth == 0) {
            bkl_owner = -1;
            spin_lock_release(&kernel_lock);
        }
    }
}

// void lock_kernel()
// {
//     /* TODO: P3-TASK3 multicore*/
//     spin_lock_acquire(&kernel_lock);
// }

// void unlock_kernel()
// {
//     /* TODO: P3-TASK3 multicore*/
//     spin_lock_release(&kernel_lock);
// }
