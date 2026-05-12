#include <asm.h>
#include <csr.h>
#include <asm/regs.h>

// pcb.full_context 指向完整上下文
// pcb.kerlnel_sp 指向实时使用的真实栈顶
// switchto_context.sp 存储进入switch_to前的栈顶

.macro SAVE_CONTEXT
  /* TODO: [p2-task3] save all general purpose registers here! */
  /* HINT: Pay attention to the function of tp and sp, and save them carefully! */
  // 进入时，sp指向用户栈栈顶

  sd sp, PCB_USER_SP(tp) // 保存用户栈到pcb的user_stack成员中 √
  ld sp, PCB_CONTEXT(tp) // 从pcb中读取完整上下文的位置

  # sd sp, PCB_USER_SP(tp) // 保存用户栈到pcb的user_stack成员中 √
  # ld sp, PCB_KERNEL_SP(tp) // 从pcb中读取kernel_stack √
  # addi sp, sp, -OFFSET_SIZE 
  # sd sp, PCB_KERNEL_SP(tp) // 更新pcb的kernel_stack成员（真实栈顶） √
  # sd sp, PCB_CONTEXT(tp) // 记录完整上下文的位置

  // 保存GPRs
  sd zero, OFFSET_REG_ZERO(sp)
  sd ra, OFFSET_REG_RA(sp)
  sd gp, OFFSET_REG_GP(sp)
  sd tp, OFFSET_REG_TP(sp)
  sd t0, OFFSET_REG_T0(sp)
  sd t1, OFFSET_REG_T1(sp)
  sd t2, OFFSET_REG_T2(sp)
  sd s0, OFFSET_REG_S0(sp)
  sd s1, OFFSET_REG_S1(sp)
  sd a0, OFFSET_REG_A0(sp)
  sd a1, OFFSET_REG_A1(sp)
  sd a2, OFFSET_REG_A2(sp)
  sd a3, OFFSET_REG_A3(sp)
  sd a4, OFFSET_REG_A4(sp)
  sd a5, OFFSET_REG_A5(sp)
  sd a6, OFFSET_REG_A6(sp)
  sd a7, OFFSET_REG_A7(sp)
  sd s2, OFFSET_REG_S2(sp)
  sd s3, OFFSET_REG_S3(sp)
  sd s4, OFFSET_REG_S4(sp)
  sd s5, OFFSET_REG_S5(sp)
  sd s6, OFFSET_REG_S6(sp)
  sd s7, OFFSET_REG_S7(sp)
  sd s8, OFFSET_REG_S8(sp)
  sd s9, OFFSET_REG_S9(sp)
  sd s10, OFFSET_REG_S10(sp)
  sd s11, OFFSET_REG_S11(sp)
  sd t3, OFFSET_REG_T3(sp)
  sd t4, OFFSET_REG_T4(sp)
  sd t5, OFFSET_REG_T5(sp)
  sd t6, OFFSET_REG_T6(sp)
    
  /*
   * Disable user-mode memory access as it should only be set in the
   * actual user copy routines.
   *
   * Disable the FPU to detect illegal usage of floating point in kernel
   * space.
   */
  # li      t0, ~SR_SUM & ~SR_FS
  # csrr    t1, sstatus
  # and     t0, t0, t1
  # csrw    sstatus, t0
  li      t0, SR_SUM
  csrs    sstatus, t0

  /* TODO: [p2-task3] save sstatus, sepc, stval and scause on kernel stack */
  // 保存CSRs
  csrr t0, sstatus
  csrr t1, sepc
  csrr t2, stval
  csrr t3, scause
  sd t0, OFFSET_REG_SSTATUS(sp)
  sd t1, OFFSET_REG_SEPC(sp)
  sd t2, OFFSET_REG_SBADADDR(sp)
  sd t3, OFFSET_REG_SCAUSE(sp)
 
.endm

// 恢复完整上下文
.macro RESTORE_CONTEXT
  /* TODO: Restore all general purpose registers and sepc, sstatus */
  /* HINT: Pay attention to sp again! */
  ld sp, PCB_CONTEXT(tp) // 让sp退回到完整上下文位置

  ld t0, OFFSET_REG_SSTATUS(sp)
  ld t1, OFFSET_REG_SEPC(sp)
  ld t2, OFFSET_REG_SBADADDR(sp)
  ld t3, OFFSET_REG_SCAUSE(sp)

  csrw sstatus, t0
  csrw sepc, t1
  csrw stval, t2
  csrw scause, t3

  ld zero, OFFSET_REG_ZERO(sp)
  ld ra, OFFSET_REG_RA(sp)
  ld gp, OFFSET_REG_GP(sp)
  ld tp, OFFSET_REG_TP(sp) 
  ld t0, OFFSET_REG_T0(sp)
  ld t1, OFFSET_REG_T1(sp)
  ld t2, OFFSET_REG_T2(sp)
  ld s0, OFFSET_REG_S0(sp)
  ld s1, OFFSET_REG_S1(sp)
  ld a0, OFFSET_REG_A0(sp)
  ld a1, OFFSET_REG_A1(sp)
  ld a2, OFFSET_REG_A2(sp)
  ld a3, OFFSET_REG_A3(sp)
  ld a4, OFFSET_REG_A4(sp)
  ld a5, OFFSET_REG_A5(sp)
  ld a6, OFFSET_REG_A6(sp)
  ld a7, OFFSET_REG_A7(sp)
  ld s2, OFFSET_REG_S2(sp)
  ld s3, OFFSET_REG_S3(sp)
  ld s4, OFFSET_REG_S4(sp)
  ld s5, OFFSET_REG_S5(sp)
  ld s6, OFFSET_REG_S6(sp)
  ld s7, OFFSET_REG_S7(sp)
  ld s8, OFFSET_REG_S8(sp)
  ld s9, OFFSET_REG_S9(sp)
  ld s10, OFFSET_REG_S10(sp)
  ld s11, OFFSET_REG_S11(sp)
  ld t3, OFFSET_REG_T3(sp)
  ld t4, OFFSET_REG_T4(sp)
  ld t5, OFFSET_REG_T5(sp)
  ld t6, OFFSET_REG_T6(sp)

  # ld sp , PCB_KERNEL_SP(tp) // 恢复真实栈顶
  ld sp, PCB_USER_SP(tp) // 从pcb的user_stack成员中恢复用户栈（切回内核后需要重新save到pcb）
  
  # addi sp, sp, OFFSET_SIZE // 清理内核栈
  # sd sp, PCB_KERNEL_SP(tp) // 将内核栈地址保存到pcd的kernel_stack成员（在save前不会再使用）
  # ld sp, PCB_USER_SP(tp) // 从pcb的user_stack成员中恢复用户栈（切回内核后需要重新save到pcb）
.endm

ENTRY(enable_preempt)
  not t0, x0
  csrs CSR_SIE, t0
  jr ra
ENDPROC(enable_preempt)

ENTRY(disable_preempt)
  csrw CSR_SIE, zero
  jr ra
ENDPROC(disable_preempt)

ENTRY(enable_interrupt)
  li t0, SR_SIE
  csrs CSR_SSTATUS, t0
  jr ra
ENDPROC(enable_interrupt)

ENTRY(disable_interrupt)
  li t0, SR_SIE
  csrc CSR_SSTATUS, t0
  jr ra
ENDPROC(disable_interrupt)

// NOTE: the address of previous pcb in a0
// NOTE: the address of next pcb in a1
ENTRY(switch_to)
// 只切换内核上下文
  // 进入时，sp就是当前进程内核栈栈顶
  mv t0, sp // 这里的sp是压入switchto_context前的值
  addi t0, t0, -(SWITCH_TO_SIZE) // 分配switchto_context空间
  // t0指向switchto_context

  /* TODO: [p2-task1] save all callee save registers on kernel stack,
   * see the definition of `struct switchto_context` in sched.h*/
  sd ra, SWITCH_TO_RA(t0)
  # ld sp, PCB_KERNEL_SP(a0) // pcb里的sp，才是指向完整上下文的
  sd sp, SWITCH_TO_SP(t0)  // 这里的sp是进入switch_to前的栈顶，需要在restore最后恢复
  sd s0, SWITCH_TO_S0(t0)
  sd s1, SWITCH_TO_S1(t0)
  sd s2, SWITCH_TO_S2(t0)
  sd s3, SWITCH_TO_S3(t0)
  sd s4, SWITCH_TO_S4(t0)
  sd s5, SWITCH_TO_S5(t0)
  sd s6, SWITCH_TO_S6(t0)
  sd s7, SWITCH_TO_S7(t0)
  sd s8, SWITCH_TO_S8(t0)
  sd s9, SWITCH_TO_S9(t0)
  sd s10, SWITCH_TO_S10(t0)
  sd s11, SWITCH_TO_S11(t0)
  sd t0, PCB_KERNEL_SP(a0) // 使pcb中的kernel_sp指向switchto_context（与下面对应）

  /* TODO: [p2-task1] restore all callee save registers from kernel stack,
   * see the definition of `struct switchto_context` in sched.h*/
  ld t0, PCB_KERNEL_SP(a1) // 转到下一个进程的kernel_stack（指向下一个进程的switchto_context）
  ld ra, SWITCH_TO_RA(t0)
  ld sp, SWITCH_TO_SP(t0) // 这里的sp是压入switchto_context前的值（也是真实栈顶）
  ld s0, SWITCH_TO_S0(t0)
  ld s1, SWITCH_TO_S1(t0)
  ld s2, SWITCH_TO_S2(t0)
  ld s3, SWITCH_TO_S3(t0)
  ld s4, SWITCH_TO_S4(t0)
  ld s5, SWITCH_TO_S5(t0)
  ld s6, SWITCH_TO_S6(t0)
  ld s7, SWITCH_TO_S7(t0)
  ld s8, SWITCH_TO_S8(t0)
  ld s9, SWITCH_TO_S9(t0)
  ld s10, SWITCH_TO_S10(t0)
  ld s11, SWITCH_TO_S11(t0)

  // 无需清理，sp是压入switchto_context前的值
  # addi sp, t0, SWITCH_TO_SIZE // 清理switchto_context空间
  sd sp, PCB_KERNEL_SP(a1) // 更新pcb中的kernel_sp（真实栈顶）
  jr ra // 假现场会跳到ret_from_exception
ENDPROC(switch_to)

ENTRY(ret_from_exception)
  # call enable_preempt // TODO: 现有逻辑中存在错误关中断的现象
  /* TODO: [p2-task3] restore context via provided macro and return to sepc */
  /* HINT: remember to check your sp, does it point to the right address? */
  call	unlock_kernel
  RESTORE_CONTEXT 
  sret  // 返回sepc
ENDPROC(ret_from_exception)

ENTRY(exception_handler_entry)
  // TODO: 保存用户栈到pcb中
  /* TODO: [p2-task3] save context via the provided macro */
  SAVE_CONTEXT // sp指向刚刚保存的regs_context_t，且这个值被存到了pcb的kernel_stack成员中
  call	lock_kernel

  /* TODO: [p2-task3] load ret_from_exception into $ra so that we can return to
   * ret_from_exception when interrupt_help complete.*/
  /* TODO: [p2-task3] call interrupt_helper
   * NOTE: don't forget to pass parameters for it.*/ 
  //* 参数见kernel/irq/irq.c中 interrupt_helper 函数定义
  mv a0, sp // 指向刚刚保存的regs_context_t
  csrr a1, stval // Supervisor bad address or instruction，提供了异常发生时的具体错误地址或值
  csrr a2, scause
  call interrupt_helper

  // 这里sp又重新指向了regs_context_t？？？
  //try: 直接读取pcb的kernel_stack成员肯定没问题
  ld sp, PCB_KERNEL_SP(tp)
  la ra, ret_from_exception
  jr ra 

ENDPROC(exception_handler_entry)
