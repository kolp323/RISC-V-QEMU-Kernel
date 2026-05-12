#ifndef PROC_H // Include Guard
#define PROC_H

#include <common.h>
#include <os/sched.h>
#include <os/task.h>

#define BATCH 1
#define SINGLE 0
#define MAX_STATUS_LEN 7

#define STATUS2STR(KILLED, STATUS) \
    (((STATUS) == TASK_BLOCKED && (KILLED)) ? "ZOMBIE" : \
     (STATUS) == TASK_BLOCKED ? "BLOCKED" : \
     (STATUS) == TASK_READY ? "READY  " : \
     (STATUS) == TASK_RUNNING ? "RUNNING" : \
     (STATUS) == TASK_EXITED ? "EXITED " : "UNKNOWN")


extern void ret_from_exception();

#ifdef S_CORE
pid_t do_exec(int id, int argc, uint64_t arg0, uint64_t arg1, uint64_t arg2);
#else
pid_t do_exec(char *name, int argc, char *argv[]);
#endif
void do_exit(void);
void force_exit(pcb_t *pcb_ptr);
int do_kill(pid_t pid);
int do_waitpid(pid_t pid);
void do_process_show();
pid_t do_getpid();
void do_taskset(int mask, char *name, int pid, int argc, char **argv);
pid_t do_thread_create(ptr_t start_routine, ptr_t arg, int arg_bytes);
pid_t do_thread_join(pid_t pid);

#endif