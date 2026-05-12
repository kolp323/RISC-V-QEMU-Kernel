#include <os/mm.h>
#include <pgtable.h>
#include <os/string.h>
#include <os/kernel.h> // for bios_sd_read/write
#include <os/swap.h>
#include <os/proc.h>
#include <printk.h>
#include <assert.h>

active_page_t active_pages[MAX_ALLOWABLE_PAGES]; // 循环队列
int swap_page_status[SWAP_MAX_PAGES]; // swap区域各页状态，是否被占用
static int clock_head = 0; // Clock 算法指针
int active_page_count = 0;
unsigned int SWAP_START_SEC; // 交换空间起始扇区数

void init_swap(){
    int *swap_offset_ptr = (int *)(BOOTBLOCK_BASE + SWAP_OFFSET_LOC);
    int swap_offset = *swap_offset_ptr;
    SWAP_START_SEC = swap_offset / SECTOR_SIZE;
    for (int i = 0; i < SWAP_MAX_PAGES; i++) {
        swap_page_status[i] = FREE;
    }
    for (int i = 0; i < MAX_ALLOWABLE_PAGES; i++) {
        active_pages[i].valid = 0;
    }
    printl("[init_Swap]...\n");
    printl("  SWAP_MAX_PAGES:%d\n", SWAP_MAX_PAGES);
    printl("  MAX_ALLOWABLE_PAGES:%d\n", MAX_ALLOWABLE_PAGES);
}
// 返回一个无效槽位
active_page_t * get_free_active_page_slot(){
    for (int i = 0; i < MAX_ALLOWABLE_PAGES; i++) {
        if(!active_pages[i].valid) return &active_pages[i];
    }
    return NULL;
}
int get_free_swap_id(){
    for (int i = 0; i < SWAP_MAX_PAGES; i++) {
        if (swap_page_status[i] == FREE){
            swap_page_status[i] = OCCUPIED; // 标记为占用
            return i;
        }
    }
    return -1;
}

// 将现有的物理页加入 Clock 算法的 active_list
void add_to_active_list(uintptr_t pa, uintptr_t va, int pid)
{
    active_page_t *ptr; // 指向新加入页的管理项

    // 检查 active_pages 是否已满
    if (active_page_count < MAX_ALLOWABLE_PAGES) {
        // 未满：直接记录进一个空闲槽位
        ptr = get_free_active_page_slot();
        if (ptr == NULL) return;
        active_page_count++;
    } else {
        // 已满：必须通过 swap_out 踢出一个受害者来腾出位置
        int victim_idx = swap_out();
        
        if (victim_idx == -1) {
            // 极端情况：Swap 磁盘也满了
            // 放弃换出
            printl("[Error] Swap full! Page at VA 0x%lx (PID %d) cannot be added to active list.\n", va, pid);
            // TODO：是否应该exit？？
            // page_kva = allocPage(1);
            do_exit();
            return;
        }
        
        // 复用受害者的槽位
        ptr = &active_pages[victim_idx];
    }

    // 记录新页面的数据
    ptr->pid = pid;
    ptr->vaddr = va;
    ptr->kva = pa2kva(pa); // 物理地址转内核虚拟地址
    ptr->valid = 1;
    
    // 获取并保存 PTE 指针
    // 此时 map_page 已经执行完毕，PTE 肯定存在
    PTE *pte = get_pte(va, current_running->pgdir_kva);
    if (pte == NULL) {
        printl("[Error] PTE not found for VA 0x%lx during add_to_active_list\n", va);
        // 回滚 active_page_count，此页置为无效
        ptr->valid = 0;
        active_page_count--;
        return;
    }
    ptr->pte = pte;
}

// 从active_pages移除对应页的记录
void remove_from_active_list(uintptr_t page_kva){
    int idx = -1;
    // 遍历寻找对应的 active_page
    for (int i = 0; i < MAX_ALLOWABLE_PAGES; i++) {
        if (active_pages[i].kva == page_kva) {
            idx = i;
            break;
        }
    }

    // 如果没找到（可能不是用户数据页，或者未被记录），直接返回
    if (idx == -1) return;
    // 置为无效
    active_pages[idx].valid = 0;
    // 更新计数
    active_page_count--;

    // 如果active_page_count不为0，且clock_head 正好指向被移除的页，需要指向下一个有效页
    if (active_page_count > 0 && clock_head == idx) {
        while (!active_pages[clock_head].valid) clock_head = (clock_head + 1) % MAX_ALLOWABLE_PAGES;
    }
}

// kva: 待交换的内存页起始虚地址
// swap_index: 存入到交换区的页编号(目前已经存储到的第几个swap页)
void swap_write(uintptr_t kva, uint64_t swap_index) {
    uint32_t sector = SWAP_START_SEC + swap_index * SECTOR_NUMS_PER_PAGE;
    bios_sd_write(kva2pa(kva), SECTOR_NUMS_PER_PAGE, sector);
}

// kva: 待交换的内存页起始虚地址
// swap_index: 从交换区读出的页编号
void swap_read(uintptr_t kva, uint64_t swap_index) {
    uint32_t sector = SWAP_START_SEC + swap_index * SECTOR_NUMS_PER_PAGE;
    bios_sd_read(kva2pa(kva), SECTOR_NUMS_PER_PAGE, sector);
}


// 从 active_pages 中选一页换出到磁盘
// 返回值：腾出的物理页在active_pages中的索引
// 返回-1无效
int swap_out() {
    int victim_idx = -1;
    // 能进入swap_out肯定是active_page_count满了
    assert(active_page_count >= MAX_ALLOWABLE_PAGES);
    // Clock 算法寻找被替换页
    while (1) {
        // 跳过无效页
        while (!active_pages[clock_head].valid) clock_head = (clock_head + 1) % MAX_ALLOWABLE_PAGES;

        PTE *pte = active_pages[clock_head].pte;
        // 如果 A 位为 1，给予第二次机会，清零 A 位并移动指针
        if (*pte & _PAGE_ACCESSED) {
            // 清除 A 位 (取出flags，将A位清零，写回)
            set_attribute(pte, get_attribute(*pte, FLAGS_MASK) & ~_PAGE_ACCESSED);
            // 刷新 TLB，因为修改了页表项
            local_flush_tlb_all(); 
        } else {
            // 找到 A=0 的页面，作为被替换页
            victim_idx = clock_head;
            break;
        }
        // 寻找下一个候选页
        clock_head = (clock_head + 1) % MAX_ALLOWABLE_PAGES;
    }

//* 处理被替换页
    active_page_t *victim = &active_pages[victim_idx];
    PTE *victim_pte = victim->pte;
    uintptr_t victim_kva = victim->kva;
    // 生成 Swap id
    uint64_t swap_id = get_free_swap_id(); 
    if (swap_id == -1) {
        printl("[WARNING] swap space not enough, forgive swap operation.\n");
        return swap_id;
    }
    // 将数据写入磁盘
    swap_write(victim_kva, swap_id);
    printl("[Swap Out] PID: %d, VA: %lx -> SwapID: %d\n", victim->pid, victim->vaddr, swap_id);

    // TODO: 换出时，修改原 PTE：V=0, S=1, PFN 字段存 swap_id
    // ! 清空现有属性，保留 Swap ID 到高位 (原 PFN 位置)
    *victim_pte = (swap_id << _PAGE_PFN_SHIFT) | _PAGE_SWAP; 
    local_flush_tlb_all();

    // 释放被换出的物理页
    freePage(victim_kva);
    
    // 更新 clock 指针
    clock_head = (clock_head + 1) % MAX_ALLOWABLE_PAGES;
    
    return victim_idx;
}

// 将swap_id号的磁盘页框读入到内存kva页中
void swap_in(uintptr_t kva, uint64_t swap_id){
    // 从磁盘读取数据到新分配的物理页
    swap_read(kva, swap_id);
    // 释放 Swap 空间对应的槽位
    swap_page_status[swap_id] = FREE;
}