#include <os/list.h> // 包含列表操作的头文件，通常用于管理任务队列
#include <os/sched.h> // 包含调度相关的头文件，如任务控制块 (pcb_t) 的定义和调度操作
#include <type.h>     // 包含基本类型定义的头文件，如 uint64_t

uint64_t time_elapsed = 0; // 全局变量：用于存储从某个基准点开始的 CPU 周期数（原始计时器值）
uint64_t time_base = 0;    // 全局变量：时间基准，通常是每秒的 CPU 周期数（时钟频率），用于将周期数转换为秒或毫秒

uint64_t get_ticks() // 函数：获取原始的 CPU 周期数/时间计数器的值
{
    // 内联汇编：用于读取特定的硬件寄存器
    __asm__ __volatile__(
        "rdtime %0"    // RISC-V 指令：将时间计数器 (time/mtime) 的值读入到 %0 对应的 C 变量中
        : "=r"(time_elapsed)); // 输出操作数：将 time_elapsed 变量绑定到汇编中的 %0，"=r" 表示可写入的通用寄存器
    return time_elapsed; // 返回读取到的 CPU 周期数
}

uint64_t get_timer() // 函数：获取基于时间基准的时钟值（例如，秒或毫秒）
{
    // 将原始的 CPU 周期数除以时间基准，转换为更易理解的时间单位
    return get_ticks() / time_base;
}

uint64_t get_time_base() // 函数：获取时间基准（时钟频率）
{
    return time_base; // 返回时间基准
}

void latency(uint64_t time) // 函数：实现一个忙等待的延迟 (latency)
{
    uint64_t begin_time = get_timer(); // 记录开始时的时间值（使用 get_timer 转换后的值）

    // 忙等待循环：一直循环直到当前时间减去开始时间达到或超过指定的延迟时间 'time'
    while (get_timer() - begin_time < time);
    return; 
}

void check_sleeping(void)
{
    // TODO: [p2-task3] Pick out tasks that should wake up from the sleep queue
    
    list_node_t *node = sleep_queue.next;
    uint64_t current_time = get_timer();
    while (node != &sleep_queue) {
        pcb_t *pcb_ptr = CONTAINER_OF(node, pcb_t, list);
        list_node_t *next_node = node->next;
        if (pcb_ptr->wakeup_time <= current_time) {
            do_unblock(node); // 唤醒任务
        } 
        node = next_node; // 继续检查下一个节点
    }
    
}