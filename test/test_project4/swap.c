#include <stdio.h>
#include <unistd.h>

#define PAGE_SIZE 4096
// 请确保内核中 swap.h 的 MAX_ALLOWABLE_PAGES 也设置为 4
#define MAX_MEM_PAGES 4 

// 测试总页数：填满(4) + 溢出1(1) + 再次溢出1(1) = 6页
#define TEST_PAGES (MAX_MEM_PAGES + 2) 

long* base_addr = (long*)0x40000000;

void write_page(int page_idx) {
    int offset = (page_idx * PAGE_SIZE) / sizeof(long);
    // 写入特定的验证数据：页号 + 偏移标志
    base_addr[offset] = (long)page_idx; 
    base_addr[offset + 1] = (long)page_idx + 0x12345678;
}

void verify_page(int page_idx) {
    int offset = (page_idx * PAGE_SIZE) / sizeof(long);
    long val1 = base_addr[offset];
    long val2 = base_addr[offset + 1];
    
    long expect1 = (long)page_idx;
    long expect2 = (long)page_idx + 0x12345678;

    if (val1 != expect1 || val2 != expect2) {
        printf("\n[ERROR] Data Corruption at Page %d!\n", page_idx);
        printf("  Expected: %ld, %ld\n", expect1, expect2);
        printf("  Got:      %ld, %ld\n", val1, val2);
        // while(1); // 可以在此死循环以便调试
    }
}

int main() {
    sys_move_cursor(0, 0);
    printf("=== Clock Algo & Data Consistency Test ===\n");

    // 1. 填满物理内存 [P0, P1, P2, P3]
    printf("[1/4] Filling memory (P0-%d)...\n", MAX_MEM_PAGES - 1);
    for (int i = 0; i < MAX_MEM_PAGES; i++) {
        write_page(i);
    }

    // 2. 写入 P4, 触发第一次 Swap [P1, P2, P3, P4]  -> P0
    printf("[2/4] Write P%d (Trigger Swap, Expect Evict P0)...\n", MAX_MEM_PAGES);
    write_page(MAX_MEM_PAGES);

    // 3. 给予 P1 第二次机会 [P2, P3, P4, P1]  
    printf("[3/4] Read P1 (Give 2nd Chance)...\n");
    volatile long tmp = base_addr[(1 * PAGE_SIZE) / sizeof(long)]; 
    (void)tmp;

    // 4. 写入 P5, 触发第二次 Swap [P3, P4, P1, P5]  -> P2
    printf("[4/4] Write P%d (Trigger Swap, Expect Skip P1 -> Evict P2)...\n", MAX_MEM_PAGES + 1);
    write_page(MAX_MEM_PAGES + 1);

    // 5. 最终一致性检查 从左向右依次被换出  -> P3 P4 P1 P5 P0
    printf("[Final] Verifying ALL data consistency...\n");
    for (int i = 0; i < TEST_PAGES; i++) {
        verify_page(i);
    }

    printf("\n=== Test Finished (Check Kernel Log for Swap Sequence) ===\n");
    return 0;
}