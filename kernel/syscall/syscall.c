#include <sys/syscall.h>
#include <asm/unistd.h>
#include <os/kernel.h>
#include <screen.h>
#include <os/time.h>
#include <os/task.h>
#include <os/proc.h>
#include <os/mm.h>
#include <os/pipe.h>
#include <os/net.h>
#include <os/fs.h>

long (*syscall[NUM_SYSCALLS])();

void handle_syscall(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
    /* TODO: [p2-task3] handle syscall exception */
    /**
     * HINT: call syscall function like syscall[fn](arg0, arg1, arg2),
     * and pay attention to the return value and sepc
     */
    // 执行到ecall时硬件会自动将 ecall 指令本身的地址保存到 sepc
    regs->sepc += 4; // 确保 sret 返回到 ecall 指令的下一条指令
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-non-prototype"
    regs->regs[10] = syscall[regs->regs[17]](   // x17: a7 
        regs->regs[10],                         // x10: a0 
        regs->regs[11],                         // x11: a1 
        regs->regs[12],                         // x12: a2 
        regs->regs[13],                         // x13: a3 
        regs->regs[14]                          // x14: a4 
    );
    #pragma GCC diagnostic pop
}

void init_syscall(void)
{
    // TODO: [p2-task3] initialize system call table.
    // 有关系统调用的宏在 include/asm/unistd.h 中
    // 函数声明在 include/os/sched.h 中
    syscall[SYSCALL_EXEC] = (long (*)()) do_exec;
    syscall[SYSCALL_EXIT] = (long (*)()) do_exit;
    syscall[SYSCALL_SLEEP] = (long (*)()) do_sleep;
    syscall[SYSCALL_KILL] = (long (*)()) do_kill;
    syscall[SYSCALL_WAITPID] = (long (*)()) do_waitpid;
    syscall[SYSCALL_PS] = (long (*)()) do_process_show;
    syscall[SYSCALL_GETPID] = (long (*)()) do_getpid;
    syscall[SYSCALL_YIELD] = (long (*)()) do_scheduler;
    syscall[SYSCALL_SET_SCHE_WORKLOAD] = (long (*)()) do_set_sche_workload;
    // 函数声明在 include/os/kernel.h
    syscall[SYSCALL_READCH] = (long (*)()) bios_getchar; // shell从串口读取输入
    // 函数声明在 drivers/screen.h 中
    syscall[SYSCALL_WRITE] = (long (*)()) screen_write;
    syscall[SYSCALL_CURSOR] = (long (*)()) screen_move_cursor;
    syscall[SYSCALL_REFLUSH] = (long (*)()) screen_reflush;
    syscall[SYSCALL_CLEAR] = (long (*)()) screen_clear;
    // 函数声明在 include/os/time.h 中
    syscall[SYSCALL_GET_TIMEBASE] = (long (*)()) get_time_base;
    syscall[SYSCALL_GET_TICK] = (long (*)()) get_ticks;
    // 函数声明在 include/os/lock.h 中
    syscall[SYSCALL_LOCK_INIT] = (long (*)()) do_mutex_lock_init;
    syscall[SYSCALL_LOCK_ACQ] = (long (*)()) do_mutex_lock_acquire;
    syscall[SYSCALL_LOCK_RELEASE] = (long (*)()) do_mutex_lock_release;
    syscall[SYSCALL_BARR_INIT] = (long (*)()) do_barrier_init;
    syscall[SYSCALL_BARR_WAIT] = (long (*)()) do_barrier_wait;
    syscall[SYSCALL_BARR_DESTROY] = (long (*)()) do_barrier_destroy;
    syscall[SYSCALL_COND_INIT] = (long (*)()) do_condition_init;
    syscall[SYSCALL_COND_WAIT] = (long (*)()) do_condition_wait;
    syscall[SYSCALL_COND_SIGNAL] = (long (*)()) do_condition_signal;
    syscall[SYSCALL_COND_BROADCAST] = (long (*)()) do_condition_broadcast;
    syscall[SYSCALL_COND_DESTROY] = (long (*)()) do_condition_destroy;
    syscall[SYSCALL_MBOX_OPEN] = (long (*)()) do_mbox_open;
    syscall[SYSCALL_MBOX_CLOSE] = (long (*)()) do_mbox_close;
    syscall[SYSCALL_MBOX_SEND] = (long (*)()) do_mbox_send;
    syscall[SYSCALL_MBOX_RECV] = (long (*)()) do_mbox_recv;
    syscall[SYSCALL_TASKSET] = (long (*)()) do_taskset;

    // [p3-task5] 函数声明在 include/os/sched.h 中
    syscall[SYSCALL_THREAD_CREATE] = (long (*)()) do_thread_create;
    syscall[SYSCALL_THREAD_JOIN] = (long (*)()) do_thread_join;

    // [p4-task4 task5] 函数声明在 include/os/mm.h 中
    syscall[SYSCALL_SHM_GET] = (long (*)()) shm_page_get;
    syscall[SYSCALL_SHM_DT] = (long (*)()) shm_page_dt;
    syscall[SYSCALL_FREE_MEM] = (long (*)()) get_free_memory;
    syscall[SYSCALL_PIPE_OPEN] = (long (*)()) sys_pipe_open;
    syscall[SYSCALL_PIPE_GIVE] = (long (*)()) sys_pipe_give_pages;
    syscall[SYSCALL_PIPE_TAKE] = (long (*)()) sys_pipe_take_pages;

    // ===自定义功能
    // 展示任务
    syscall[SYSCALL_SHOW_TASK] = (long (*)()) list_tasks;

    // [p5] 函数声明在 include/os/net.h 中
    syscall[SYSCALL_NET_SEND] = (long (*)()) do_net_send;
    syscall[SYSCALL_NET_RECV] = (long (*)()) do_net_recv;
    syscall[SYSCALL_NET_RECV_STREAM] = (long (*)()) do_net_recv_stream;
    syscall[SYSCALL_RESET_STREAM] = (long (*)()) do_reset_stream;


    // 函数声明在 include/os/fs.h 中
    syscall[SYSCALL_FS_MKFS] = (long (*)()) do_mkfs;
    syscall[SYSCALL_FS_STATFS] = (long (*)()) do_statfs;
    syscall[SYSCALL_FS_CD] = (long (*)()) do_cd;
    syscall[SYSCALL_FS_MKDIR] = (long (*)()) do_mkdir;
    syscall[SYSCALL_FS_RMDIR] = (long (*)()) do_rmdir;
    syscall[SYSCALL_FS_LS] = (long (*)()) do_ls;
    syscall[SYSCALL_FS_OPEN] = (long (*)()) do_open;
    syscall[SYSCALL_FS_READ] = (long (*)()) do_read;
    syscall[SYSCALL_FS_WRITE] = (long (*)()) do_write;
    syscall[SYSCALL_FS_CLOSE] = (long (*)()) do_close;
    syscall[SYSCALL_FS_LN] = (long (*)()) do_ln;
    syscall[SYSCALL_FS_RM] = (long (*)()) do_rm;
    syscall[SYSCALL_FS_LSEEK] = (long (*)()) do_lseek;

    // syscall[SYSCALL_FS_TOUCH] = (long (*)()) ;
    // syscall[SYSCALL_FS_CAT] = (long (*)()) ;
}