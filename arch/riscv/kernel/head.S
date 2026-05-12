/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <asm.h>
#include <csr.h>

// ！！！！！！！！需要同步修改sched.h
#define REGS_CONTEXT_SIZE 288
#define PAGE_SIZE 4096 
#define KERNEL_STACK_1		0xffffffc050500000lu - REGS_CONTEXT_SIZE  // 预留完整上下文空间  
#define KERNEL_STACK_0		KERNEL_STACK_1 - PAGE_SIZE * 100  // 主核栈顶：为从核留100页 5049 BEE0

.section ".entry_function","ax"
ENTRY(_start)
  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

	csrr 	a0, CSR_MHARTID
	bnez 	a0, secondary

  /* TODO: [p1-task2] clear BSS for flat non-ELF images */
  la t0, __bss_start   // 注：加载链接器提供的符号地址时要使用la
  la t1, __BSS_END__
clear_bss:
  bge t0, t1, end_clear_bss 
  sd zero, 0(t0)
  addi t0, t0, 8
  j clear_bss
end_clear_bss:

  /* TODO: [p1-task2] setup C environment */
  li sp, KERNEL_STACK_0
  call main

secondary:
	li sp, KERNEL_STACK_1
	call 	main

loop:
  wfi
  j loop

END(_start)
