#include <os/mm.h>
#include <pgtable.h>
#include <os/string.h>
#include <os/swap.h>
#include <os/proc.h>
#include <printk.h>

typedef struct {
    int key;            // 用户传递的唯一键值
    uintptr_t va;       // 使用的用户虚拟地址
    uintptr_t pa;       // 对应的物理页地址
    int ref_count;      // 引用计数：有多少个进程映射了此页
    int valid;          // 该槽位是否有效，无效则空闲
} shm_t;

static shm_t shm_table[SHM_MAX_NUM];

void init_shm(){
    for (int i = 0; i < SHM_MAX_NUM; i++) {
        shm_table[i].va = SHM_VA_BASE + i * PAGE_SIZE; // 定死每个共享页的va
        shm_table[i].ref_count = 0;
        shm_table[i].valid = 0;
    }
}

uintptr_t shm_page_get(int key)
{
    // TODO [P4-task4] shm_page_get:
    int i;
    int free_idx = -1;
    // 查找 Key 是否已存在
    for (i = 0; i < SHM_MAX_NUM; i++) {
        if (shm_table[i].valid && shm_table[i].key == key) {
            // 映射已有的物理页到当前进程的 va
            shm_map_page(shm_table[i].va, shm_table[i].pa, current_running->pgdir_kva);
            // 增加引用计数
            shm_table[i].ref_count++;
            return shm_table[i].va;
        }
        if (!shm_table[i].valid && free_idx == -1) {
            free_idx = i; // 同时记录空闲槽位
        }
    }

    // Key 不存在，创建新的共享页
    if (free_idx != -1) {
        ptr_t kva = allocPage(1);
        uintptr_t pa = kva2pa(kva);
        shm_table[free_idx].key = key;
        shm_table[free_idx].pa = pa;
        shm_table[free_idx].ref_count = 1;
        shm_table[free_idx].valid = 1;

        // 映射到当前进程
        shm_map_page(shm_table[free_idx].va, pa, current_running->pgdir_kva);
        return shm_table[free_idx].va;
    }

    // 表满了
    return 0;
}

// detach
void shm_page_dt(uintptr_t addr)
{
    // TODO [P4-task4] shm_page_dt:
    uintptr_t pgdir_kva = current_running->pgdir_kva;
    PTE *pte = get_pte(addr, pgdir_kva);
    
    if (pte == NULL || !(*pte & _PAGE_PRESENT)) {
        return; // 无效地址
    }

    // 根据传入的虚拟地址，反查物理地址
    for (int i = 0; i < SHM_MAX_NUM; i++) {
        if (shm_table[i].valid && shm_table[i].va == addr) {
            shm_table[i].ref_count--;
            
            // 如果没人用了，回收物理页
            if (shm_table[i].ref_count == 0) {
                shm_table[i].valid = 0;
                // 将物理页转回 KVA 进行释放
                freePage(pa2kva(shm_table[i].pa)); 
            }
            break;
        }
    }

    // 解除映射 (清空 PTE)
    *pte = 0; 
    local_flush_tlb_page(addr);
}

// 将指定的物理地址 pa 映射到虚拟地址 va
void shm_map_page(uintptr_t va, uintptr_t pa, uintptr_t pgdir)
{
    // 关键在设置共享标志
    map_page(va, pa, pgdir, _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC | _PAGE_USER | _PAGE_SHM);
}