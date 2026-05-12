#include <os/ioremap.h>
#include <os/mm.h>
#include <pgtable.h>
#include <type.h>

// maybe you can map it to IO_ADDR_START ?
static uintptr_t io_base = IO_ADDR_START;

void *ioremap(unsigned long phys_addr, unsigned long size)
{
    // TODO: [p5-task1] map one specific physical region to virtual address
    uintptr_t vaddr_start = io_base;
    // 确保确保物理地址和大小都是页对齐的
    uint64_t offset_in_page = phys_addr & (LARGE_PAGE_SIZE - 1); // 计算pa页内偏移
    ptr_t pa_aligned = ROUNDDOWN(phys_addr, LARGE_PAGE_SIZE); // 向下对齐物理地址到 2MB 边界
    uint64_t map_size = ROUNDDOWN(size + offset_in_page + LARGE_PAGE_SIZE - 1, LARGE_PAGE_SIZE);
    
    // 标志位：可读、可写
    uint64_t flags = _PAGE_READ | _PAGE_WRITE;
    // 逐页建立映射
    for (uint64_t offset = 0; offset < map_size; offset += LARGE_PAGE_SIZE) {
        uintptr_t vaddr = vaddr_start + offset;
        uintptr_t paddr = pa_aligned + offset;
        map_page_kernel(vaddr, paddr, pa2kva(PGDIR_PA), flags);
    }

    io_base += map_size; // 更新 io_base
    local_flush_tlb_all();

    // 返回映射后的虚拟首地址
    return (void *)(vaddr_start + offset_in_page);
    
    // return NULL;
}

void iounmap(void *io_addr)
{
    // TODO: [p5-task1] a very naive iounmap() is OK
    // maybe no one would call this function?
}
