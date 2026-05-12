#ifndef PGTABLE_H
#define PGTABLE_H

#include <type.h>

#define SATP_MODE_SV39 8
#define SATP_MODE_SV48 9

#define SATP_ASID_SHIFT 44lu
#define SATP_MODE_SHIFT 60lu

#define NORMAL_PAGE_SHIFT 12lu
#define NORMAL_PAGE_SIZE (1lu << NORMAL_PAGE_SHIFT)
#define LARGE_PAGE_SHIFT 21lu
#define LARGE_PAGE_SIZE (1lu << LARGE_PAGE_SHIFT)

/*
 * Flush entire local TLB.  'sfence.vma' implicitly fences with the instruction
 * cache as well, so a 'fence.i' is not necessary.
 */
static inline void local_flush_tlb_all(void)
{
    __asm__ __volatile__ ("sfence.vma" : : : "memory");
}

/* Flush one page from local TLB */
static inline void local_flush_tlb_page(unsigned long addr)
{
    __asm__ __volatile__ ("sfence.vma %0" : : "r" (addr) : "memory");
}

static inline void local_flush_icache_all(void)
{
    asm volatile ("fence.i" ::: "memory");
}

static inline void set_satp(
    unsigned mode, unsigned asid, unsigned long ppn)
{
    unsigned long __v =
        (unsigned long)(((unsigned long)mode << SATP_MODE_SHIFT) | ((unsigned long)asid << SATP_ASID_SHIFT) | ppn);
    __asm__ __volatile__("sfence.vma\ncsrw satp, %0" : : "rK"(__v) : "memory");
}

#define PGDIR_PA 0x51000000lu  // use 51000000 page as PGDIR

/*
 * PTE format:
 * | XLEN-1  10 | 9             8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
 *       PFN      reserved for SW   D   A   G   U   X   W   R   V
 */

#define _PAGE_ACCESSED_OFFSET 6

#define _PAGE_PRESENT (1 << 0)  /* Valid */
#define _PAGE_READ (1 << 1)     /* Readable */
#define _PAGE_WRITE (1 << 2)    /* Writable */
#define _PAGE_EXEC (1 << 3)     /* Executable */
#define _PAGE_USER (1 << 4)     /* User */
#define _PAGE_GLOBAL (1 << 5)   /* Global */
#define _PAGE_ACCESSED (1 << 6) /* Set by hardware on any access */
#define _PAGE_DIRTY (1 << 7)    /* Set by hardware on any write */
// #define _PAGE_SOFT (1 << 8)     /* Reserved for software */
#define _PAGE_SWAP (1 << 8)     /*[p4-task3] Swapped out to disk */
// 如果 V=0 且 S=0：页面无效（从未分配），触发缺页异常 -> 执行 Task 2 的按需分配。
// ?? 如果 V=0 且 S=1：页面被换出（在磁盘），触发缺页异常 -> 执行 Task 3 的 Swap In。
// TODO：保留 Swap ID 到高位 (原 PFN 位置)
#define _PAGE_SHM (1 << 9)     /*[p4-task5] shm */


#define _PAGE_PFN_SHIFT 10lu
#define FLAGS_MASK ((1lu << _PAGE_PFN_SHIFT) - 1)

#define VA_MASK ((1lu << 39) - 1)

#define PPN_BITS 9lu
#define NUM_PTE_ENTRY (1 << PPN_BITS)
#define GET_VPN2(va) (((va) >> (NORMAL_PAGE_SHIFT + PPN_BITS*2)) & 0x1FF)
#define GET_VPN1(va) (((va) >> (NORMAL_PAGE_SHIFT + PPN_BITS)) & 0x1FF)
#define GET_VPN0(va) (((va) >> (NORMAL_PAGE_SHIFT)) & 0x1FF)

typedef uint64_t PTE;

/* Shared Kernel Virtual Address offset */
#define KVA_PA_OFFSET 0xffffffc000000000lu


/* Translation between physical addr and kernel virtual addr */
static inline uintptr_t kva2pa(uintptr_t kva)
{
    /* TODO: [P4-task1] */
    return kva - KVA_PA_OFFSET;
}

static inline uintptr_t pa2kva(uintptr_t pa)
{
    /* TODO: [P4-task1] */
    return pa + KVA_PA_OFFSET;
}

/* get physical page addr from PTE 'entry' */
static inline uint64_t get_pa(PTE entry)
{
    /* TODO: [P4-task1] */
    // PFN 在 PTE 的 [53:10] 位
    // 规定高位的保留位全0
    return ((entry >> _PAGE_PFN_SHIFT) << NORMAL_PAGE_SHIFT);
}

/* Get/Set page frame number of the `entry` */
// 获取物理页号
static inline long get_pfn(PTE entry)
{
    /* TODO: [P4-task1] */
    return (entry >> _PAGE_PFN_SHIFT);
}
static inline void set_pfn(PTE *entry, uint64_t pfn)
{
    /* TODO: [P4-task1] */
    // 保留[9:0]位flags
    uint64_t flags = *entry & ((1lu << _PAGE_PFN_SHIFT) - 1);
    // 设置新fpn
    *entry = (pfn << _PAGE_PFN_SHIFT) | flags;
}

/* Get/Set attribute(s) of the `entry` */
static inline long get_attribute(PTE entry, uint64_t mask)
{
    /* TODO: [P4-task1] */
    return entry & mask;
}
// 假设是将[9:0]标志位设置为bits
static inline void set_attribute(PTE *entry, uint64_t bits)
{
    /* TODO: [P4-task1] */
    // 假设是将[9:0]标志位设置为bits
    *entry = (*entry & ~((1lu << _PAGE_PFN_SHIFT) - 1)) | bits;
    // // 假设是添加/设置标志位
    // *entry |= bits;
}

static inline void clear_pgdir(uintptr_t pgdir_addr)
{
    /* TODO: [P4-task1] */
    long *table = (long *)pgdir_addr;
    for (int i = 0; i < NUM_PTE_ENTRY; i++) { // NUM_PTE_ENTRY = 512
        table[i] = 0;
    }
}

// 查找虚拟地址对应的页表项指针
// 如果中间页表不存在，返回 NULL
static inline PTE *get_pte(uintptr_t va, uintptr_t pgdir_kva) {
    uint64_t vpn2 = GET_VPN2(va);
    uint64_t vpn1 = GET_VPN1(va);
    uint64_t vpn0 = GET_VPN0(va);

    PTE *pgd = (PTE *)pgdir_kva;
    if ((pgd[vpn2] & _PAGE_PRESENT) == 0) return NULL;

    PTE *pmd = (PTE *)pa2kva(get_pa(pgd[vpn2]));
    if ((pmd[vpn1] & _PAGE_PRESENT) == 0) return NULL;

    PTE *pte = (PTE *)pa2kva(get_pa(pmd[vpn1]));
    return &pte[vpn0];
}

#endif  // PGTABLE_H
