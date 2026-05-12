#include <os/irq.h>
#include <os/mm.h>
#include <os/task.h>
#include <printk.h>
#include <os/proc.h>
#include <os/swap.h>
#include <os/string.h>

int is_valid_uva(uint64_t stval){
    // 获取缺页地址所在的页基址 (页对齐)
    uintptr_t fault_va = ROUNDDOWN(stval, PAGE_SIZE);

    // 检查 text、data、bss 段范围 ---
    task_info_t *task_info = get_task_info(current_running->name);
    // 如果找不到 task_info，认为是IDLE进程，暂且放行
    if (task_info == NULL) {
        return 1;
    }
    uint64_t va_start = task_info->vaddr;
    uint64_t va_end = va_start + task_info->memsz;
    if (fault_va >= va_start && fault_va < va_end) {
        return 1;  // 合法访问：属于程序自身的代码或数据
    }

    // 检查用户栈范围
    uint64_t user_stack_top = USER_STACK_ADDR;
    uint64_t user_stack_base = USER_STACK_ADDR - PAGE_SIZE * USER_PAGE_NUM;
    if (fault_va >= user_stack_base && fault_va < user_stack_top){
        return 1;  // 合法访问：属于用户栈
    }

    return 0; // 非法访问
}
void handle_invalid_access(regs_context_t *regs, uint64_t stval, uint64_t scause){
    printk("[ERROR] Accessing invalid space at 0x%p!!!\n", stval);
    do_exit(); // 释放资源并退出
}

// 返回新建立页的kva
uintptr_t handle_page_fault(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
    uintptr_t pgdir_kva = current_running->pgdir_kva;
    // TODO:什么时候应该检查合法性？？？
    // 区分按需分配和非法访问
    // // 检查缺页地址(stval)是否合法
    // if(!is_valid_uva(stval)){
    //     handle_invalid_access(regs, stval, scause);
    //     return;
    // }
    
    // 检查stval所在页是否被swap到磁盘上
    // 检查页表项，swap的页一定只有第三级页表是无效的
    // 若中间页表无效(返回NULL)则就是缺页
    PTE *pte = get_pte(stval, pgdir_kva); 
    uint64_t swap_id;
    int if_swap_in = 0; //是否进行换入操作
    if (pte != NULL 
        &&  !get_attribute(*pte, _PAGE_PRESENT)
        &&   get_attribute(*pte, _PAGE_SWAP)) {
        if_swap_in = 1;
        swap_id = (*pte) >> _PAGE_PFN_SHIFT; // 从pte中读取swap_id
    }
    // 在内存中开辟此页空间，并建立映射
    uintptr_t kva = alloc_page_helper(stval, pgdir_kva, current_running->pid);

    if (kva == 0) {
        // 分配失败，或者映射失败
        printk("[FATAL] Page Fault: Memory allocation failed for VA 0x%lx\n", stval);
        do_exit(); // 释放资源并退出
    }
    
    // 若需要换入，则从磁盘中恢复数据
    if (if_swap_in) {
        swap_in(kva, swap_id);
        printl("[Swap In] PID: %d, VA: %lx <- SwapID: %lx\n", 
               current_running->pid, stval, swap_id);
    }
    else {
        // 若并不是从 Swap 恢复，而是全新的映射，清空内存
        memset((void *)kva, 0, PAGE_SIZE); 
    }
    local_flush_tlb_page(stval);
    // local_flush_tlb_all();
    return kva;
}