#ifndef __ASM_UNISTD_H__
#define __ASM_UNISTD_H__

#define SYSCALL_EXEC 0
#define SYSCALL_EXIT 1
#define SYSCALL_SLEEP 2
#define SYSCALL_KILL 3
#define SYSCALL_WAITPID 4
#define SYSCALL_PS 5
#define SYSCALL_GETPID 6
#define SYSCALL_YIELD 7
#define SYSCALL_WRITE 20
#define SYSCALL_READCH 21
#define SYSCALL_CURSOR 22
#define SYSCALL_REFLUSH 23
#define SYSCALL_CLEAR 24
#define SYSCALL_GET_TIMEBASE 30
#define SYSCALL_GET_TICK 31
#define SYSCALL_LOCK_INIT 40
#define SYSCALL_LOCK_ACQ 41
#define SYSCALL_LOCK_RELEASE 42
#define SYSCALL_SHOW_TASK 43
#define SYSCALL_BARR_INIT 44
#define SYSCALL_BARR_WAIT 45
#define SYSCALL_BARR_DESTROY 46
#define SYSCALL_COND_INIT 47
#define SYSCALL_COND_WAIT 48
#define SYSCALL_COND_SIGNAL 49
#define SYSCALL_COND_BROADCAST 50
#define SYSCALL_COND_DESTROY 51
#define SYSCALL_MBOX_OPEN 52
#define SYSCALL_MBOX_CLOSE 53
#define SYSCALL_MBOX_SEND 54
#define SYSCALL_MBOX_RECV 55
#define SYSCALL_SHM_GET 56
#define SYSCALL_SHM_DT 57
#define SYSCALL_FREE_MEM 58
#define SYSCALL_PIPE_OPEN 59
#define SYSCALL_PIPE_GIVE 60
#define SYSCALL_PIPE_TAKE 61

#define SYSCALL_NET_SEND 63
#define SYSCALL_NET_RECV 64

#define SYSCALL_FS_MKFS 65
#define SYSCALL_FS_STATFS 66
#define SYSCALL_FS_CD 67
#define SYSCALL_FS_MKDIR 68
#define SYSCALL_FS_RMDIR 69
#define SYSCALL_FS_LS 70
#define SYSCALL_FS_TOUCH 71
#define SYSCALL_FS_CAT 72
#define SYSCALL_FS_OPEN 73
#define SYSCALL_FS_READ 74 
#define SYSCALL_FS_WRITE 75 
#define SYSCALL_FS_CLOSE 76
#define SYSCALL_FS_LN 77
#define SYSCALL_FS_RM 78
#define SYSCALL_FS_LSEEK 79

#define SYSCALL_NET_RECV_STREAM 90
#define SYSCALL_RESET_STREAM 91

#define SYSCALL_SET_SCHE_WORKLOAD 92
#define SYSCALL_TASKSET 93
#define SYSCALL_THREAD_CREATE 94
#define SYSCALL_THREAD_JOIN 95
/// syscall long int (*[96])()

// arch/riscv/include/asm/unistd.h（内核使用）
// kernel/syscall/syscall.c 中填充系统调用表

// tiny_libc/include/syscall.h（用户使用）
// tiny_libc/include/unistd.h 中添加系统调用函数声明
// tiny_libc/syscall.c 中添加系统调用函数定义

// 新增系统调用号的话，注意保证两个文件内的调用号一致。
#endif