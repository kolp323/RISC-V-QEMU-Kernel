#include <asm.h>

# ! 在main.c中，传递栈顶地址
# TODO: 尝试用a1或者其他寄存器来传递app的栈顶地址
# ! -------------------------------------

.section ".entry_function","ax"
ENTRY(_start)

    /* TODO: [p1-task3] setup C runtime environment for the user program */
    la t0, __bss_start   
    la t1, __BSS_END__
clear_bss:
    bge t0, t1, end_clear_bss 
    sb zero, 0(t0)
    addi t0, t0, 1
    j clear_bss

end_clear_bss:
//[p2-task1] 和 p1用户栈的设置方式冲突
    # mv t0, sp           // t0暂存kernel栈指针 
    # mv sp, a1           //* sp移至APP栈顶，完成栈切换（在main.c的exec_app()中执行任务前会将程序栈顶地址放在a1寄存器中）   
    # addi sp, sp, -16    // 为 ra 和 kernel sp 腾出空间
    # sd ra, 8(sp)        // 保存返回kernel的地址
    # sd t0, 0(sp)        // 保存 kernel 栈指针

    # // 设置栈指针
    # // 调用bios_sd_read时a0存储着当前app的起始地址
    # la sp, APP_STACK_TOP
    # add sp, sp, a0

    /* TODO: [p1-task3] enter main function */
    call main   // call main 时 ra 会被覆盖为返回 crt0.S 的下一个指令地址

    # /* TODO: [p1-task3] finish task and return to the kernel, replace this in p3-task2! */
    # ld ra, 8(sp)            // 恢复返回kernel的地址
    # ld sp, 0(sp)            // 恢复kernel栈指针
    # ret

    /************************************************************/
    /* TODO: [P3] call sys_exit */
    /************************************************************/
    call sys_exit
// while(1) loop, unreachable here
loop:
    wfi
    j loop

END(_start)
