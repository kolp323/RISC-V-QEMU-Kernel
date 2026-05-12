#include <os/mm.h>
#include <pgtable.h>
#include <os/string.h>
#include <os/swap.h>
#include <os/proc.h>
#include <printk.h>

// NOTE: A/C-core
static ptr_t kernMemCurr = FREEMEM_KERNEL;
ptr_t freePageList = 0; // 空闲链表头
static int free_page_list_count = 0; // 空闲链表中的页数

// 返回值为分配空间的低地址
ptr_t allocPage(int numPage)
{
    // 只有申请 1 页时才从空闲链表分配，保证多页分配的连续性
    if (numPage == 1 && freePageList != 0) {
        // 取出链表头
        ptr_t ret = freePageList;
        // 链表头指向下一页
        freePageList = *(ptr_t *)ret;
        free_page_list_count--;
        // 清空回收页的内容！
        memset((void *)ret, 0, PAGE_SIZE);

        return ret;
    }

    // align PAGE_SIZE
    ptr_t ret = ROUND(kernMemCurr, PAGE_SIZE);
    kernMemCurr = ret + numPage * PAGE_SIZE;
    return ret;
}

// NOTE: Only need for S-core to alloc 2MB large page
#ifdef S_CORE
static ptr_t largePageMemCurr = LARGE_PAGE_FREEMEM;
ptr_t allocLargePage(int numPage)
{
    // align LARGE_PAGE_SIZE
    ptr_t ret = ROUND(largePageMemCurr, LARGE_PAGE_SIZE);
    largePageMemCurr = ret + numPage * LARGE_PAGE_SIZE;
    return ret;    
}
#endif

// 应当传入kva
void freePage(ptr_t baseAddr)
{
    // TODO [P4-task1] (design you 'freePage' here if you need):
    // 简单检查地址是否合法
    if (baseAddr == 0) return;

    // 将当前空闲链表的头节点地址，写入到要把baseAddr回收的页的起始位置
    // 即当前回收页的开头指向上一个空闲页
    *(ptr_t *)baseAddr = freePageList;
    // 更新头节点指向当前页
    freePageList = baseAddr;
    free_page_list_count++;
}

void *kmalloc(size_t size)
{
    // TODO [P4-task1] (design you 'kmalloc' here if you need):
}


/* this is used for mapping kernel virtual address into user page table */
// 为了让用户进程在陷入内核时能正常运行（访问内核代码和数据），每个用户进程的页表中都必须包含 内核页表 的映射 。
// 做法：将已有的内核页表完整地拷贝到用户页表中
void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir)
{
    // TODO [P4-task1] share_pgtable:
    // 将已有的内核页表完整地拷贝到用户页表中
    memcpy((void *)dest_pgdir, (void *)src_pgdir, PAGE_SIZE);
}

int cur_program_size(){
    for (int i = 0; i < TASK_MAXNUM; i++){
        if (strcmp(current_running->name, tasks[i].name) == 0) {
            return tasks[i].memsz;
        }
    }
    return 0;
}

/* allocate physical page for `va`, mapping it into `pgdir`,
   return the kernel virtual address for the page
   */
// 为va（用户地址空间）分配物理页，并建立其到va的映射。中间检查页表有效性并进行分配
// 返回va对应物理页的kva（内核地址空间），给内核直接操作这个物理页数据的机会
uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir, int pid)
{
    // TODO [P4-task1] alloc_page_helper:
    // 分配一个新的物理页
    ptr_t page_kva = allocPage(1);
    uintptr_t page_pa = kva2pa(page_kva);
    // 清空新分配页
    memset((void *)page_kva, 0, PAGE_SIZE);
    // 建立va到page_pa的映射，map_page 内部会自动加上 Accessed | Dirty
    map_page(va, page_pa, pgdir, _PAGE_PRESENT | _PAGE_READ |
         _PAGE_WRITE | _PAGE_EXEC | _PAGE_USER);
    
    // // 若不是内核页，需要注册到swap管理中
    // if (va <= USER_MAX_VA && current_running->pid > 0) {
    //     // pin住栈数据和代码数据
    //     int is_stack = (va >= USER_STACK_ADDR - PAGE_SIZE); // 保护栈数据
    //     int is_text = (va <= USER_ENTRYPOINT + cur_program_size()); // 保护代码段

    //     // 只有满足条件的页才会被加入 Clock 算法队列
    //     if (!is_text && !is_stack) {
    //         // 如果满了，会自动 swap_out 一个旧页，腾出位置记录这个新页
    //         add_to_active_list(page_pa, va, pid);
    //     }
    //     // add_to_active_list(page_pa, va, pid);
    // }
    return page_kva;
}

long get_free_memory(void)
{
    long total_free = 0;
    // 线性空间
    // 如果 kernMemCurr 还没有越界
    if (kernMemCurr < MEMORY_SIZE_END) {
        total_free += MEMORY_SIZE_END - kernMemCurr;
    }

    // 空闲链表空间
    total_free += free_page_list_count * PAGE_SIZE;
    return total_free;
}

// 通用函数：将物理地址 pa 映射到虚拟地址 va
// flags: 除Valid、Accessed、Dirty以外的标志 (如 _PAGE_READ | _PAGE_WRITE | _PAGE_USER ...)
void map_page(uintptr_t va, uintptr_t pa, uintptr_t pgdir, uint64_t flags)
{
    uint64_t vpn2 = GET_VPN2(va);
    uint64_t vpn1 = GET_VPN1(va);
    uint64_t vpn0 = GET_VPN0(va);

    PTE *pgd = (PTE *)pgdir;
    // 检查一级页表，不存在则分配
    if ((pgd[vpn2] & _PAGE_PRESENT) == 0) {
        ptr_t new_page = allocPage(1);
        set_pfn(&pgd[vpn2], kva2pa(new_page) >> NORMAL_PAGE_SHIFT);
        set_attribute(&pgd[vpn2], _PAGE_PRESENT);
        clear_pgdir(new_page);
    }

    PTE *pmd = (PTE *)pa2kva(get_pa(pgd[vpn2]));
    // 检查二级页表，不存在则分配
    if ((pmd[vpn1] & _PAGE_PRESENT) == 0) {
        ptr_t new_page = allocPage(1);
        set_pfn(&pmd[vpn1], kva2pa(new_page) >> NORMAL_PAGE_SHIFT);
        set_attribute(&pmd[vpn1], _PAGE_PRESENT);
        clear_pgdir(new_page);
    }

    // 设置三级页表，建立映射
    PTE *pte = (PTE *)pa2kva(get_pa(pmd[vpn1]));
    
    set_pfn(&pte[vpn0], pa >> NORMAL_PAGE_SHIFT);
    // 使用传入的 flags
    set_attribute(&pte[vpn0], flags | _PAGE_PRESENT | _PAGE_ACCESSED | _PAGE_DIRTY);
    local_flush_tlb_page(va);
}

// 内核专用映射函数
void map_page_kernel(uintptr_t va, uintptr_t pa, uintptr_t pgdir, uint64_t flags)
{
    uint64_t vpn2 = GET_VPN2(va);
    uint64_t vpn1 = GET_VPN1(va);

    PTE *pgd = (PTE *)pgdir;
    // 检查一级页表，不存在则分配
    if ((pgd[vpn2] & _PAGE_PRESENT) == 0) {
        ptr_t new_page = allocPage(1);
        set_pfn(&pgd[vpn2], kva2pa(new_page) >> NORMAL_PAGE_SHIFT);
        set_attribute(&pgd[vpn2], _PAGE_PRESENT);
        clear_pgdir(new_page);
    }

    PTE *pmd = (PTE *)pa2kva(get_pa(pgd[vpn2]));
    // 在二级页表建立映射
    set_pfn(&pmd[vpn1], pa >> NORMAL_PAGE_SHIFT);
    // 使用传入的 flags
    set_attribute(&pmd[vpn1], flags | _PAGE_PRESENT | _PAGE_ACCESSED | _PAGE_DIRTY);
    local_flush_tlb_page(va);
}