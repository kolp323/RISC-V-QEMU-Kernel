/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                                   Memory Management
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
#ifndef MM_H
#define MM_H

#include <type.h>
#include <pgtable.h>

#define KERNEL_PAGE_NUM 8 // 每个内核栈分配4页
#define USER_PAGE_NUM 4 // 每个用户栈分配4页

#define MAP_KERNEL 1
#define MAP_USER 2
#define MEM_SIZE 32
#define PAGE_SIZE 4096 // 4K
#define INIT_KERNEL_STACK 0xffffffc052000000
#define FREEMEM_KERNEL (INIT_KERNEL_STACK+PAGE_SIZE)

/* Rounding; only works for n = power of two */
#define ROUND(a, n)     (((((uint64_t)(a))+(n)-1)) & ~((n)-1))
#define ROUNDDOWN(a, n) (((uint64_t)(a)) & ~((n)-1))

extern ptr_t allocPage(int numPage);
// TODO [P4-task1] */
void freePage(ptr_t baseAddr);

// #define S_CORE
// NOTE: only need for S-core to alloc 2MB large page
#ifdef S_CORE
#define LARGE_PAGE_FREEMEM 0xffffffc056000000
#define USER_STACK_ADDR 0x400000
extern ptr_t allocLargePage(int numPage);
#else
// NOTE: A/C-core
#define USER_STACK_ADDR 0xf00010000
#define USER_ENTRYPOINT 0x10000
#endif

// TODO [P4-task1] */
// 定义内核空间的起始索引 (SV39 PGD 中第 256 项开始为内核)
#define KERNEL_PTE_INDEX_START 0x100
#define USER_MAX_VA 0x3FFFFFFFFF
extern void* kmalloc(size_t size);
extern void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir);
extern uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir, int pid);
void free_all_pages(uintptr_t pgdir_kva);
extern ptr_t freePageList; // 空闲链表头

// TODO [P4-task4]: shm_page_get/dt */
#define SHM_MAX_NUM 16 // 最大共享页数
#define SHM_VA_BASE  0x90000000 // 共享内存的起始虚拟地址 (用户空间)
void init_shm();
void map_page(uintptr_t va, uintptr_t pa, uintptr_t pgdir, uint64_t flags);
void map_page_kernel(uintptr_t va, uintptr_t pa, uintptr_t pgdir, uint64_t flags);
uintptr_t shm_page_get(int key);
void shm_page_dt(uintptr_t addr);
void shm_map_page(uintptr_t va, uintptr_t pa, uintptr_t pgdir);

// [p4-task4] 查看可用内存
// 物理内存范围: 0x50000000 ~ 0x60000000
// 内核虚拟地址: 0xffffffc050000000 ~ 0xffffffc060000000
#define MEMORY_SIZE_END 0xffffffc060000000 // 定义动态内存分配的结束地址
long get_free_memory(void);
#endif /* MM_H */
