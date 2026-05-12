#ifndef SWAP_H // 记得加 Include Guard
#define SWAP_H

#include <os/sched.h>
// TODO: 同步修改createimage.c
#define SWAP_SIZE (256 * 1024 * 1024) // swap区域的大小
#define MAX_ALLOWABLE_PAGES 4 // 限制允许分配的物理页数，强行触发换页测试！
#define SWAP_OFFSET_LOC 0x1ed // swap区域在镜像中的起始位置
#define SECTOR_NUMS_PER_PAGE 8 // 一页4KB = 8 * 512B = 8个扇区
#define SWAP_MAX_PAGES (SWAP_SIZE/PAGE_SIZE) // SWAP空间可容纳的最大页数
// 16K页

// 管理现有的用户页信息，用于实现clock替换算法
typedef struct {
    uintptr_t kva;      // TODO:(实际用到)物理页的内核虚拟地址
    PTE *pte;           // TODO:(实际用到)指向该物理页对应的（三级）页表项 (用于修改 V/S 位)
    uint64_t vaddr;     // 该页对应的用户虚地址
    pid_t pid;          // 持有该页的进程 ID
    int valid;
} active_page_t;

#define FREE 0
#define OCCUPIED 1

extern unsigned int SWAP_START_SEC;
extern int swap_page_status[SWAP_MAX_PAGES]; // 16K * 4B = 64KB = 16页
extern active_page_t active_pages[MAX_ALLOWABLE_PAGES];

extern int active_page_count;

void init_swap();
int get_free_swap_id();
void swap_write(uintptr_t kva, uint64_t swap_index);
void swap_read(uintptr_t kva, uint64_t swap_index);
int swap_out();
void swap_in(uintptr_t kva, uint64_t swap_id);
void add_to_active_list(uintptr_t pa, uintptr_t va, int pid);
void remove_from_active_list(uintptr_t page_kva);

#endif