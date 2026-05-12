/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *        Process scheduling related content, such as: scheduler, process blocking,
 *                 process wakeup, process creation, process kill, etc.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#ifndef INCLUDE_SCHEDULER_H_
#define INCLUDE_SCHEDULER_H_

#include <type.h>
#include <os/list.h>
#include <os/lock.h>
#include <os/task.h>

#define REGS_CONTEXT_SIZE 288
// #define KERNEL_STACK 0x50500000 - REGS_CONTEXT_SIZE // 预留完整上下文空间
#define PAGE_SIZE 4096 
// ! 需要和head.S保持一致
#define KERNEL_STACK_1		0xffffffc050500000lu - REGS_CONTEXT_SIZE  // 预留完整上下文空间  
#define KERNEL_STACK_0		KERNEL_STACK_1 - PAGE_SIZE * 100  // 主核栈顶：为从核留100页 5049 BEE0


/* used to save register infomation */
typedef struct regs_context
{
    /* Saved main processor registers.*/
    reg_t regs[32];

    /* Saved special registers. */
    reg_t sstatus;
    reg_t sepc;
    reg_t sbadaddr;
    reg_t scause;
} regs_context_t;

/* used to save register infomation in switch_to */
typedef struct switchto_context
{
    /* Callee saved registers.*/
    reg_t regs[14];
} switchto_context_t;

typedef enum {
    TASK_BLOCKED,
    TASK_RUNNING,
    TASK_READY,
    TASK_EXITED,
} task_status_t;

// ### 2. 进程控制块 (PCB)
/* Process Control Block */
typedef struct pcb
{
    /* register context */
    // NOTE: this order must be preserved, which is defined in regs.h!!
    reg_t kernel_sp; // 始终指向switchto_context  （switch_to中的sp存储切换前的内核栈sp位置）
    reg_t user_sp;
    reg_t full_context; // 始终指向完整上下文
    ptr_t kernel_stack_base; // 内核栈底
    ptr_t user_stack_base;   // 用户栈底

    /* previous, next pointer */
    list_node_t list;
    list_head wait_list;

    /* process id */
    pid_t pid;
    char name[MAX_NAME_LEN];

    /* BLOCK | READY | RUNNING */
    task_status_t status;

    /* cursor position */
    int cursor_x;
    int cursor_y;

    /* time(seconds) to wake up sleeping PCB */
    uint64_t wakeup_time;

    // [p2-task5] workload for scheduler
    int next_target;
    int progress;   

    // [p3-task4] taskset
    int killed; // 是否已经被杀死
    uint64_t cpuid;
    int mask; // 第i位为1表示仅能运行在核心 i
    
    // [p3-task5] thread
    // 线程组标识，为父进程的pid // 初始化为0，表示是一个独立进程
    // 线程之间由pid区分
    int thread_group_id; 

    // [p4-task1]
    uintptr_t pgdir_kva; // 存储页目录的内核虚拟地址，方便内核访问和管理 

    // [p6-task1]
    uint32_t dir_inode;
} pcb_t;

// ### 3. 调度器接口与管理队列

// 创建一个指向0的结构体指针，通过访问成员地址来得知成员的偏移量
#define OFFSET_OF(TYPE, MEMBER) ( (long) &((TYPE *)0)->MEMBER )
// 由一个指向成员member的指针ptr，获取其所在结构体type的起始位置
#define CONTAINER_OF(ptr, type, member) ( \
    (type *)((char *)(ptr) - OFFSET_OF(type, member)) \
)


/* ready queue to run */
extern list_head ready_queue;

/* sleep queue to be blocked in */
extern list_head sleep_queue;

/* current running task PCB */
register pcb_t * current_running asm("tp");
extern pid_t process_id;

extern pcb_t pcb[TASK_MAXNUM];
#define NR_CPUS 2
extern pcb_t kpcb[NR_CPUS];
extern pcb_t pid0_pcb;
extern const ptr_t pid0_stack;

#define TEMP_BUF (0x59000000 + KVA_PA_OFFSET + \
    (current_running->pid % TASK_MAXNUM) * PAGE_SIZE * 4) // 非对齐读取时的暂时（虚拟）内存地址


extern void init_pcb(void);
extern void switch_to(pcb_t *prev, pcb_t *next);
void do_scheduler(void);
void do_sleep(uint32_t);

void do_block(list_node_t *, list_head *queue);
void do_unblock(list_node_t *);

void do_set_sche_workload(int remain_length); // [p2-task5]
/************************************************************/
/************************************************************/

#endif
