#ifndef __INCLUDE_PIPE_H__
#define __INCLUDE_PIPE_H__
#include <os/list.h>

#define PIPE_MAX_NUM 16
#define PIPE_BUFFER_SIZE 32 // 缓冲区大小 (页数)
#define PIPE_NAME_LEN 32

typedef struct {
    char name[PIPE_NAME_LEN + 1];
    int valid;
    // 存储物理地址 (PA) 的环形缓冲区
    uintptr_t page_buffer[PIPE_BUFFER_SIZE]; 
    int head; // 写指针
    int tail; // 读指针
    int data_count; // 当前管道内缓冲的页数
    list_head wait_queue; // 等待队列
} pipe_t;

void init_pipes();
int sys_pipe_open(const char *name);
long sys_pipe_give_pages(int pipe_idx, void *src, size_t length);
long sys_pipe_take_pages(int pipe_idx, void *dst, size_t length);

#endif