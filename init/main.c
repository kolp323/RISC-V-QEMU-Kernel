#include <common.h>
#include <asm.h>
#include <asm/unistd.h>
#include <os/irq.h>
#include <os/sched.h>
#include <os/workload.h>
#include <os/lock.h>
#include <os/kernel.h>
#include <os/task.h>
#include <os/pipe.h>
#include <os/string.h>
#include <os/mm.h>
#include <os/time.h>
#include <os/ioremap.h>
#include <sys/syscall.h>
#include <screen.h>
#include <e1000.h>
#include <printk.h>
// #include <assert.h>
#include <type.h>
#include <csr.h>
#include <os/smp.h>
#include <os/proc.h>
#include <os/swap.h>
#include <os/net.h>
#include <os/fs.h>
#include <plic.h>

static void init_jmptab(void)
{
    volatile long (*(*jmptab))() = (volatile long (*(*))())KERNEL_JMPTAB_BASE;

    jmptab[CONSOLE_PUTSTR]  = (volatile long (*)())port_write;
    jmptab[CONSOLE_PUTCHAR] = (volatile long (*)())port_write_ch;
    jmptab[CONSOLE_GETCHAR] = (volatile long (*)())port_read_ch;
    jmptab[SD_READ]         = (volatile long (*)())sd_read;
    jmptab[SD_WRITE]        = (volatile long (*)())sd_write;
    jmptab[QEMU_LOGGING]    = (volatile long (*)())qemu_logging;
    jmptab[SET_TIMER]       = (volatile long (*)())set_timer;
    jmptab[READ_FDT]        = (volatile long (*)())read_fdt;
    jmptab[MOVE_CURSOR]     = (volatile long (*)())screen_move_cursor;
    jmptab[PRINT]           = (volatile long (*)())printk;
    jmptab[YIELD]           = (volatile long (*)())do_scheduler;
    jmptab[MUTEX_INIT]      = (volatile long (*)())do_mutex_lock_init;
    jmptab[MUTEX_ACQ]       = (volatile long (*)())do_mutex_lock_acquire;
    jmptab[MUTEX_RELEASE]   = (volatile long (*)())do_mutex_lock_release;

    jmptab[WRITE]           = (volatile long (*)())screen_write;
    jmptab[REFLUSH]         = (volatile long (*)())screen_reflush;
    // TODO: [p2-task1] (S-core) initialize system call table.

}


/************************************************************/

/*
 * Once a CPU core calls this function,
 * it will stop executing!
 */
// static void kernel_brake(void)
// {
//     disable_interrupt();
//     while (1)
//         __asm__ volatile("wfi");
// }


static void clean_temp_map()
{
    // 2^12 * 2^9 * 2^9 = 1GB
    // 每个一级页表项映射 1GB 空间
    // VPN2 = 0      → 0x00000000 ~ 0x3FFFFFFF
    // VPN2 = 1      → 0x40000000 ~ 0x7FFFFFFF
    PTE* pgd = (PTE *)pa2kva(PGDIR_PA);
    pgd[1] = 0;
    local_flush_tlb_all();
}

int main(void)
{
    uint64_t cpu_id = get_current_cpu_id();
    // clean_temp_map();
    if (cpu_id == 0) { // 主核
        init_jmptab();      // Init jump table provided by kernel and bios(ΦωΦ)
        init_task_info();   // Init task information (〃'▽'〃)
        init_history();     // Init load history QAQAQAQ
        // init_pipe_info();   // Init pipe information (\ 'w' /)
        init_pipes();   // Init pipe information (\ 'w' /)
        init_swap(); 
        init_shm();
        init_pcb();         // Init Process Control Blocks |•'-'•) ✧
        time_base = bios_read_fdt(TIMEBASE);    // Read CPU frequency (｡•ᴗ-)_

        // // Read Flatten Device Tree (｡•ᴗ-)_
        // e1000 = (volatile uint8_t *)bios_read_fdt(ETHERNET_ADDR);
        // uint64_t plic_addr = bios_read_fdt(PLIC_ADDR);
        // uint32_t nr_irqs = (uint32_t)bios_read_fdt(NR_IRQS);
        // printk("> [INIT] e1000: %lx, plic_addr: %lx, nr_irqs: %lx.\n", e1000, plic_addr, nr_irqs);

        // // IOremap
        // plic_addr = (uintptr_t)ioremap((uint64_t)plic_addr, 0x4000 * NORMAL_PAGE_SIZE);
        // e1000 = (uint8_t *)ioremap((uint64_t)e1000, 8 * NORMAL_PAGE_SIZE);
        // printk("> [INIT] IOremap initialization succeeded.\n");

        init_locks();       // Init mlock mechanism o(´^｀)o
        init_barriers();    // Init barrier mechanism o(´^｀)o
        init_conditions();  // Init condition mechanism o(´^｀)o
        // init_semaphores(); // TODO: Init semaphore mechanism o(´^｀)o
        init_mbox();        // Init mailbox mechanism o(´^｀)o
        init_exception();   // Init interrupt (^_^)

        // // TODO: [p5-task3] Init plic
        // plic_init(plic_addr, nr_irqs);
        // printk("> [INIT] PLIC initialized successfully. addr = 0x%lx, nr_irqs=0x%x\n", plic_addr, nr_irqs);

        // // Init network device
        // e1000_init();
        // printk("> [INIT] E1000 device initialized successfully.\n");
        // init_net();
        clean_temp_map();

        init_syscall();     // Init system call table (0_0)
        init_screen();      // Init screen (QAQ)
        printk("> [INIT] CPU #%u has entered kernel with VM!\n", cpu_id);
        // TODO: [p4-task1 cont.] remove the brake and continue to start user processes.
        wakeup_other_hart();// wake up other hart (^.^)
// 这之前 0x50200000-0x51000000 都是直接使用物理地址访问
//----------------------------------------------------------------------------------------
        // clean_temp_map();
        init_fs();
        do_exec("shell", 0, NULL);
    }else{
        smp_init();
        printk("\n\n> [INIT] CPU #%u has entered kernel with VM!\n", cpu_id);
        printk("> [INIT] CORE%d setup successfully!\n", cpu_id);
        clean_temp_map();
    }

    // do_exec("shell", 0, NULL);
    // 设置时钟中断

    // printk("> [INIT] CPU #%u has entered kernel with VM!\n", cpu_id+1);
    // printk("> [INIT] CORE%d setup successfully!\n", cpu_id+1);
    bios_set_timer(get_ticks() + TIMER_INTERVAL);
    // Infinite while loop, where CPU stays in a low-power state (QAQQQQQQQQQQQ)
    while (1)
    {
        // // If you do non-preemptive scheduling, it's used to surrender control
        // do_scheduler();
        // do_exec("shell", 0, NULL);
        // If you do preemptive scheduling, they're used to enable CSR_SIE and wfi
        enable_preempt();
        asm volatile("wfi");
    }

    return 0;
}


