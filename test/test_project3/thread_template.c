#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "thread_utils.h"

#define MAX_BYTES 100

// 共享参数结构 (需要是全局或静态变量)
thread_args_t thread_args;

// 线程读函数: 执行 RECV 操作
void thread_reader_routine(ptr_t arg_addr) {
    thread_args_t *args = (thread_args_t *)arg_addr;
    // char buffer[RECV_MESSAGE_LENGTH];
    int msg = 0;
    int nbytes = 0; 
    int pos = args->print_loc + 1; // Reader 在下一行

    while (nbytes < MAX_BYTES) {
        sys_mbox_recv(args->mbox_recv_idx, &msg, sizeof(int));
        nbytes += sizeof(int);
        sys_move_cursor(0, pos);
        printf("[Reader] Read: (%d/%d) bytes", nbytes, MAX_BYTES);
        sys_sleep(1); // 稍微减慢速度以便观察
    }
    sys_exit(); // 线程退出
}
// 线程写函数: 执行 SEND 操作
void thread_writer_routine(ptr_t arg_addr) {
    thread_args_t *args = (thread_args_t *)arg_addr;
    // char buffer[SEND_MESSAGE_LENGTH] = "TEST_DATA";
    int msg = 111;
    int nbytes = 0; 
    int pos = args->print_loc + 2; // Writer 在下两行

    while (nbytes < MAX_BYTES) {
        sys_mbox_send(args->mbox_send_idx, &msg, sizeof(int));
        nbytes += sizeof(int);
        sys_move_cursor(0, pos);
        printf("[Writer] Wrote: (%d/%d) bytes", nbytes, MAX_BYTES);
        sys_sleep(1); // 稍微减慢速度以便观察
    }
    sys_exit(); // 线程退出
}


// argv[1]: MBOX1 Index, argv[2]: MBOX2 Index, argv[3]: Role (A or B)
int main(int argc, char *argv[]) {
    // 假设索引和角色信息被正确传递
    int mbox1_idx = atoi(argv[1]);
    int mbox2_idx = atoi(argv[2]);
    char role = argv[3][0];
    int base_pos = atoi(argv[4]); // 获取打印基准位置

    pid_t reader_tid, writer_tid;
    thread_args.print_loc = base_pos; // 传递位置给线程
    // sys_move_cursor(0, base_pos);
    // printf("[Process %c] PID %d Started...", role, sys_getpid());
    // 根据角色初始化参数
    if (role == 'A') {
        // A: 先从 mbox2 读取数据，再向 mbox1 发送数据；
        thread_args.mbox_recv_idx = mbox2_idx;
        thread_args.mbox_send_idx = mbox1_idx;
        // 启动线程
        reader_tid = sys_thread_create((ptr_t)thread_reader_routine, (uint64_t)&thread_args, sizeof(thread_args_t));
        writer_tid = sys_thread_create((ptr_t)thread_writer_routine, (uint64_t)&thread_args, sizeof(thread_args_t));
        sys_move_cursor(0, base_pos);
        printf("[Process %c] reader: pid %d, writer: pid %d          ", role, reader_tid, writer_tid);
    } else {
        // B: 先从 mbox1 读取数据，再向 mbox2 发送数据;
        thread_args.mbox_recv_idx = mbox1_idx;
        thread_args.mbox_send_idx = mbox2_idx;
        // 启动线程
        reader_tid = sys_thread_create((ptr_t)thread_reader_routine, (uint64_t)&thread_args, sizeof(thread_args_t));
        writer_tid = sys_thread_create((ptr_t)thread_writer_routine, (uint64_t)&thread_args, sizeof(thread_args_t));
        sys_move_cursor(0, base_pos);
        printf("[Process %c] reader: pid %d, writer: pid %d          ", role, reader_tid, writer_tid);
    }

    sys_sleep(3); // 确保线程创建完毕

    // 保持arg不被回收
    sys_thread_join(reader_tid);
    sys_thread_join(writer_tid);
    sys_move_cursor(0, base_pos);
    printf("[Process %c] PID %d Finished.", role, sys_getpid()); // 覆盖旧信息
    return 0;
}