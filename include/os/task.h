#ifndef __INCLUDE_TASK_H__
#define __INCLUDE_TASK_H__

#include <type.h>
#include <pgtable.h>

// taskinfo位置信息
#define TASK_NUM_LOC 0x1fa
#define TASKINFO_OFFSET_LOC 0x1f6
#define BATCH_OFFSET_LOC 0x1f2 // 批处理序列写入镜像中的位置
#define BOOTBLOCK_BASE 0x50200000 // bootblock在镜像中的地址
#define TEMP (0x59000000 + KVA_PA_OFFSET) // 非对齐读取时的暂时（虚拟）内存地址
// 0xffffffc059000000
#define TASK_MEM_BASE    0x52000000
#define TASK_MAXNUM      32
#define TASK_SIZE        0x10000
#define MAX_NAME_LEN     15
#define MAX_ARG_LEN     15
#define MAX_ARGS 8  // 最大参数数量

#define SECTOR_SIZE 512
#define NBYTES2SEC(nbytes) (((nbytes) / SECTOR_SIZE) + ((nbytes) % SECTOR_SIZE != 0))

#define BATCH 1
#define SINGLE 0
#define MAX_BATCH_SEQ_LEN (512-1)


/* TODO: [p1-task4] implement your own task_info_t! */
// typedef struct {
//     char name[MAX_NAME_LEN + 1];  // task name
//     uint64_t image_offset;   // App 在image中的地址 
//     int size;      // App 在镜像中的大小
// } task_info_t;//! 大小为32字节
typedef struct {
    char name[MAX_NAME_LEN + 1];
    uint64_t image_offset; // 程序在镜像中的起始偏移
    // 关键段信息 (假设每个程序只有一个 LOAD 段)
    uint64_t vaddr;        // Entry Point / Segment Vaddr
    uint64_t memsz;        // p_memsz
    uint64_t filesz;       // p_filesz
} task_info_t;

// !分开是为了避免修改create_image中写入taskinfo的逻辑 QAQ
typedef struct {
    uint64_t entrypoint;    // 已加载代码段的内存入口地址（目前就是物理地址）
    int is_loaded;          // 标记代码段是否已加载 (1: 已加载, 0: 未加载)
} load_history_t;

extern task_info_t tasks[TASK_MAXNUM];
extern load_history_t load_history[TASK_MAXNUM];


extern short tasknum;
extern int batch_offset_img;
extern int taskinfo_offset_img;

void init_task_info(void);
void init_history(void);
void list_tasks(void);
int process_input_echo(char *buffer, int max_name_len);
void load_task(void);
void exec_app(uint64_t entry_point, int is_batch);
task_info_t *get_task_info(char *name);

int is_valid_batch(char *buffer, int len);
void exec_batch(void);
void write_batch(void);
#endif