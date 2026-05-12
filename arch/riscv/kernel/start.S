/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <asm.h>
#include <csr.h>

/* maybe you need another stack for booting? */
// 两个核不会同时启动，可以复用同一套栈
.equ boot_stack_top_base, 0x50202000

.section ".bootkernel_entry","ax"
ENTRY(_boot)
  /*
   * this is the entrypoint of the kernel. 
   * Don't clear BSS here
   */

  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

  la sp, boot_stack_top_base

  csrr a0, CSR_MHARTID 

  // this will never return
  call boot_kernel


loop:
  wfi
  j loop

END(_boot)
