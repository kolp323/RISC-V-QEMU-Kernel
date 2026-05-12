#include <os/irq.h>
#include <os/time.h>
#include <os/sched.h>
#include <os/proc.h>
#include <os/workload.h> // [p2-task5]
#include <os/string.h>
#include <os/kernel.h>
#include <printk.h>
#include <assert.h>
#include <screen.h>
#include <csr.h> //* SCAUSE 异常和中断相关宏
#include <plic.h>
#include <os/net.h>

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
    // TODO: [p2-task3] & [p2-task4] interrupt handler.
    // call corresponding handler by the value of `scause`
    if (current_running->killed) {
        do_exit(); 
    }
    if (scause & SCAUSE_IRQ_FLAG) {
        // 最高位为1，是中断，调用 irq_table
        irq_table[scause & ~SCAUSE_IRQ_FLAG](regs, stval, scause);
    } else {
        // 最高位为0，是异常，调用 exc_table
        exc_table[scause](regs, stval, scause);
    }
}

void handle_irq_timer(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
    // TODO: [p2-task4] clock interrupt handler.
    // Note: use bios_set_timer to reset the timer and remember to reschedule
    bios_set_timer(get_ticks() + TIMER_INTERVAL);
    do_scheduler();

    // [p2-task5] workload scheduler
    // do_workload_scheduler();
}

void handle_irq_ext(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
    // TODO: [p5-task3] external interrupt handler.
    // Note: plic_claim and plic_complete will be helpful ...
    // 获取当前触发的中断源 ID
    int id = plic_claim();
    // 判断是否为网卡中断
    if(id == PLIC_E1000_PYNQ_IRQ || id == PLIC_E1000_QEMU_IRQ) net_handle_irq();
    else handle_other(regs, stval, scause);
    // 通知 PLIC 中断处理完成
    plic_complete(id);
}

void init_exception()
{
    /* TODO: [p2-task3] initialize exc_table */
    /* NOTE: handle_syscall, handle_other, etc.*/
    exc_table[EXCC_INST_MISALIGNED ] = handle_other;
    exc_table[EXCC_INST_ACCESS     ] = handle_other;
    exc_table[EXCC_BREAKPOINT      ] = handle_other;
    exc_table[EXCC_LOAD_ACCESS     ] = handle_other;
    exc_table[EXCC_STORE_ACCESS    ] = handle_other;
    exc_table[EXCC_SYSCALL         ] = handle_syscall;
    exc_table[EXCC_INST_PAGE_FAULT ] = (void (*)())handle_page_fault;
    exc_table[EXCC_LOAD_PAGE_FAULT ] = (void (*)())handle_page_fault;
    exc_table[EXCC_STORE_PAGE_FAULT] = (void (*)())handle_page_fault;
    /* TODO: [p2-task4] initialize irq_table */
    /* NOTE: handle_int, handle_other, etc.*/
    irq_table[IRQC_U_SOFT ] = handle_other;
    irq_table[IRQC_S_SOFT ] = handle_other;
    irq_table[IRQC_M_SOFT ] = handle_other;
    irq_table[IRQC_U_TIMER] = handle_other;
    irq_table[IRQC_S_TIMER] = handle_irq_timer; 
    irq_table[IRQC_M_TIMER] = handle_other;
    irq_table[IRQC_U_EXT  ] = handle_other;
    irq_table[IRQC_S_EXT  ] = handle_irq_ext;
    irq_table[IRQC_M_EXT  ] = handle_other;
    /* TODO: [p2-task3] set up the entrypoint of exceptions */
    setup_exception();
}

void handle_other(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
    char* reg_name[] = {
        "zero "," ra  "," sp  "," gp  "," tp  ",
        " t0  "," t1  "," t2  ","s0/fp"," s1  ",
        " a0  "," a1  "," a2  "," a3  "," a4  ",
        " a5  "," a6  "," a7  "," s2  "," s3  ",
        " s4  "," s5  "," s6  "," s7  "," s8  ",
        " s9  "," s10 "," s11 "," t3  "," t4  ",
        " t5  "," t6  "
    };
    for (int i = 0; i < 32; i += 3) {
        for (int j = 0; j < 3 && i + j < 32; ++j) {
            printk("%s : %016lx ",reg_name[i+j], regs->regs[i+j]);
        }
        printk("\n\r");
    }
    printk("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lu\n\r",
           regs->sstatus, regs->sbadaddr, regs->scause);
    printk("sepc: 0x%lx\n\r", regs->sepc);
    printk("tval: 0x%lx cause: 0x%lx\n", stval, scause);
    assert(0);
}
