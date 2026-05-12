/* RISC-V kernel boot stage */
#include <pgtable.h>
#include <asm.h>

// 定义一个宏，利用 GCC 扩展属性，将后续修饰的函数或变量强制放入 ".bootkernel" 段中。
// 这通常是为了确保这段代码在链接时处于特定的物理内存位置，或者在初始化后可以被丢弃。
#define ARRTIBUTE_BOOTKERNEL __attribute__((section(".bootkernel")))

// 定义一个函数指针类型 kernel_entry_t，它接受一个 unsigned long 参数（mhartid），无返回值。
typedef void (*kernel_entry_t)(unsigned long);

/********* setup memory mapping ***********/
static uintptr_t ARRTIBUTE_BOOTKERNEL alloc_page()
{
    // 定义一个静态变量 pg_base，初始值为 PGDIR_PA（页目录表的起始物理地址）。
    // static 意味着这个变量的值在函数调用之间会保留。
    static uintptr_t pg_base = PGDIR_PA;

    // 每次调用将地址增加 0x1000 (4096字节，即 4KB)，这是一个标准页面的大小。
    pg_base += 0x1000;

    // 返回新的页面地址。这是一种非常简单的“线性/Bump”分配器，只增不减。
    return pg_base;
}
// 页表映射函数：用于建立虚拟地址 (va) 到物理地址 (pa) 的映射
// using 2MB large page
static void ARRTIBUTE_BOOTKERNEL map_page(uint64_t va, uint64_t pa, PTE *pgdir)
{
    // 1. 虚拟地址掩码处理。RISC-V Sv39 模式只使用低 39 位，这里确保地址合法。
    va &= VA_MASK;

    // 2. 计算一级页表索引 (VPN2)。
    // RISC-V Sv39虚拟地址格式：[VPN2(9bit) | VPN1(9bit) | VPN0(9bit) | Offset(12bit)]
    // NORMAL_PAGE_SHIFT = 12, PPN_BITS = 9。
    // 右移 (12 + 9 + 9) = 30 位，提取出最高的 9 位作为一级索引。
    uint64_t vpn2 =
        va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);

    // 3. 计算二级页表索引 (VPN1)。
    // (va >> 21) 得到了 VPN2 + VPN1。
    // (vpn2 << PPN_BITS) 将 VPN2 移回去。
    // 两者异或(^) 或者相减，去掉了高位的 VPN2，只剩下 VPN1 的值。
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^
                    (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));

    // 4. 检查一级页表项（Root Page Table Entry）是否存在。
    if (pgdir[vpn2] == 0) {
        // 如果该项为空（0），说明还没有分配二级页表。
        
        // 分配一个新的物理页作为二级页表，并将其物理页号（PFN）填入当前项。
        // >> NORMAL_PAGE_SHIFT 是为了将物理地址转换为物理页号 (PFN)。
        set_pfn(&pgdir[vpn2], alloc_page() >> NORMAL_PAGE_SHIFT);
        
        // 设置页表项属性为 _PAGE_PRESENT（有效）。
        // 注意：这里没有设置 R/W/X，说明这不是叶子节点，而是指向下一级页表的目录项。
        set_attribute(&pgdir[vpn2], _PAGE_PRESENT);
        
        // 获取新分配页表的物理地址，并清空该页面的内存（防止脏数据）。
        clear_pgdir(get_pa(pgdir[vpn2]));
    }

    // 5. 获取二级页表（PMD）的物理基地址。
    // get_pa 从页表项中提取物理地址，转为 PTE 指针。
    PTE *pmd = (PTE *)get_pa(pgdir[vpn2]);

    // 6. 设置二级页表项（Leaf Entry / 大页表项）。
    // 在 vpn1 索引处，填入目标物理地址 pa 的物理页号。
    set_pfn(&pmd[vpn1], pa >> NORMAL_PAGE_SHIFT);

    // 7. 设置叶子节点属性。
    // 这里设置了 _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC (可读可写可执行)。
    // 同时也设置了 _PAGE_ACCESSED | _PAGE_DIRTY (已访问、脏位)，这是为了防止硬件首次访问时产生异常去更新这些位。
    // 因为这是二级页表项且带有 R/W/X 权限，CPU 会将其视为 2MB 的大页（不再去查 VPN0）。
    set_attribute(
        &pmd[vpn1], _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE |
                        _PAGE_EXEC | _PAGE_ACCESSED | _PAGE_DIRTY);
}

static void ARRTIBUTE_BOOTKERNEL enable_vm()
{
    // 1. 写入 satp (Supervisor Address Translation and Protection) 寄存器。
    // SATP_MODE_SV39: 开启 Sv39 分页模式。
    // 0: ASID (Address Space ID)，这里设为0。
    // PGDIR_PA >> NORMAL_PAGE_SHIFT: 将根页表的物理页号写入 PPN 字段。
    set_satp(SATP_MODE_SV39, 0, PGDIR_PA >> NORMAL_PAGE_SHIFT);

    // 2. 刷新 TLB (Translation Lookaside Buffer)。
    // 因为刚刚修改了 satp 和页表，必须刷新缓存，确保 CPU 使用新的映射关系。
    local_flush_tlb_all();
}

/* Sv-39 mode
 * 0x0000_0000_0000_0000-0x0000_003f_ffff_ffff is for user mode
 * 0xffff_ffc0_0000_0000-0xffff_ffff_ffff_ffff is for kernel mode
 */
static void ARRTIBUTE_BOOTKERNEL setup_vm()
{
    // 1. 清空根页表所在的物理页。
    clear_pgdir(PGDIR_PA);
    // map kernel virtual address(kva) to kernel physical
    // address(kpa) kva = kpa + 0xffff_ffc0_0000_0000 use 2MB page,
    // map all physical memory

    // 2. 获取根页表指针。
    PTE *early_pgdir = (PTE *)PGDIR_PA;

    // 3. [循环 1] 建立高地址内核映射 (Higher-half Kernel)。
    // 将物理地址 0x50000000 附近的内存映射到虚拟地址 0xffffffc050000000。
    // 循环步长 0x200000lu (2MB)，因为我们用的是大页。
    for (uint64_t kva = 0xffffffc050000000lu;
         kva < 0xffffffc060000000lu; kva += 0x200000lu) {
        // kva2pa(kva) 是一个宏，将虚拟地址减去偏移量得到物理地址。
        map_page(kva, kva2pa(kva), early_pgdir);
    }

    // 4. [循环 2] 建立恒等映射 (Identity Mapping)。
    // 将物理地址 0x50000000 映射到 同样的虚拟地址 0x50000000。
    // **这非常重要**：当 CPU 执行 enable_vm() 开启分页的瞬间，PC 指针还在物理地址（如 0x50001xxx）处。
    // 如果没有这个映射，开启分页后，CPU 依然按 PC 寻址，会发现该虚拟地址无效，导致立即崩溃 (Page Fault)。
    for (uint64_t pa = 0x50000000lu; pa < 0x51000000lu;
         pa += 0x200000lu) {
        map_page(pa, pa, early_pgdir);
    }

    // 5. 开启 MMU。
    enable_vm();
}

extern uintptr_t _start[]; // 声明外部符号 _start，这是内核主体的入口地址。

/*********** start here **************/
int ARRTIBUTE_BOOTKERNEL boot_kernel(unsigned long mhartid)
{
    // mhartid 是 Hardware Thread ID (CPU 核 ID)。
    
    if (mhartid == 0) {
        // 如果是主核 (Core 0)，负责初始化页表并开启分页。
        setup_vm();
    } else {
        // 如果是其他从核，直接开启分页。
        // 注意：这里在实际系统中存在竞争风险。从核必须等待主核完成 setup_vm 后才能执行 enable_vm。
        // 这里假设代码有外部同步机制或者简化了处理。
        enable_vm();
    }

    /* enter kernel */
    // 1. pa2kva(_start): 将 _start 的物理地址转换为高位虚拟地址 (例如 0xffffffc0...)。
    // 2. (kernel_entry_t): 强制转换为函数指针。
    // 3. (...)(mhartid): 调用该函数，并传入 mhartid。
    // 此时 PC 指针将跳转到高地址的内核空间执行。
    ((kernel_entry_t)pa2kva((uintptr_t)_start))(mhartid);

    return 0; // 理论上永远不会运行到这里。
}
