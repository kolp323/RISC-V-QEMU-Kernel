#include <os/mm.h>
#include <pgtable.h>
#include <os/string.h>
#include <os/swap.h>
#include <os/proc.h>
#include <printk.h>

// 递归释放页表及其映射的物理页
// pgdir: 当前级页表的基地址 (KVA)
// level: 当前页表层级
// base_va: 当前的虚拟地址前缀
void free_page_table_helper(uintptr_t pgdir, int level, uintptr_t base_va) {
    PTE *table = (PTE *)pgdir;
    
    // 遍历页表项 
    for (int i = 0; i < NUM_PTE_ENTRY; i++) {
        PTE pte = table[i];
        uintptr_t current_va = base_va + ((uintptr_t)i << (NORMAL_PAGE_SHIFT + level * PPN_BITS));
        if (current_va > USER_MAX_VA) continue;  // 禁止清理内核页
        if (i >= KERNEL_PTE_INDEX_START) continue;  // 禁止清理内核页

        if (pte & _PAGE_PRESENT) {
            // 获取下一级物理页的 PFN，并转为 KVA
            uintptr_t child_pa = (pte >> _PAGE_PFN_SHIFT) << NORMAL_PAGE_SHIFT;
            uintptr_t child_kva = pa2kva(child_pa);
            if (level > 0) {
                free_page_table_helper(child_kva, level - 1, current_va);
            }
            // level == 0，这是最后一级页表，指向物理数据页
            else if (pte & _PAGE_SHM) {
                // 若为共享页，应该调用detach而非直接释放
                shm_page_dt(current_va);
            } 
            else {
                // 回收物理数据页
                freePage(child_kva);
                
                // [P4-Task3 Swap] 需要检查 active_pages
                // 如果该页在 active_pages 队列中，需要将其移除，否则 Clock 算法会访问非法内存
                remove_from_active_list(child_kva); 
            }
        }
        // 若叶子节点页被换出，需要释放 swap 槽位
        else if (level== 0 && get_attribute(pte, _PAGE_SWAP)) {
            int swap_id = pte >> _PAGE_PFN_SHIFT;
            if (swap_id >= 0 && swap_id < SWAP_MAX_PAGES) {
                swap_page_status[swap_id] = FREE;
                printl("Freed Swap ID: %d\n", swap_id);
            }
        }
        // 清除页表项
        table[i] = 0;
    }
    freePage(pgdir);
}

// 对外接口：回收指定 PID 进程的所有页表和物理页
void free_all_pages(uintptr_t pgdir_kva) {
    // 从根页表开始递归 (SV39 根是 level 2)
    free_page_table_helper(pgdir_kva, 2, 0); 
    local_flush_tlb_all();
}