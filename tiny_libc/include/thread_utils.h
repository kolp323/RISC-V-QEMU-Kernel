#include <unistd.h>
// 线程内部同步所需的共享结构
typedef struct {
    int mbox_recv_idx;  // 接收数据的 Mailbox 索引
    int mbox_send_idx;  // 发送数据的 Mailbox 索引
    int print_loc; // 打印起始位置
} thread_args_t;

// 假设的线程启动函数指针类型
typedef void (*thread_routine_t)(ptr_t);
extern void thread_reader_routine(ptr_t arg_addr);
extern void thread_writer_routine(ptr_t arg_addr);