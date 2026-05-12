/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2015 Regents of the University of California
 这个头文件 csr.h 主要定义了 RISC-V 架构下
 与控制和状态寄存器（CSR, Control and Status Register）相关的常量和宏，
 便于操作和管理 CPU 的特权级、异常、中断、页表等功能。
 */

#ifndef CSR_H
#define CSR_H

/* Status register flags 状态寄存器相关宏*/
#define SR_SIE    0x00000002 /* Supervisor Interrupt Enable S 特权级中断使能*/
#define SR_SPIE   0x00000020 /* Previous Supervisor IE 上一次 S 特权级中断使能*/
#define SR_SPP    0x00000100 /* Previously Supervisor 上一次 S 特权级*/
#define SR_SUM    0x00040000 /* Supervisor User Memory Access S 特权级允许访问 U 特权级内存*/
// 用于操作 sstatus 等寄存器的各类标志位，控制中断、特权级、浮点和扩展状态等
#define SR_FS           0x00006000 /* Floating-point Status */
#define SR_FS_OFF       0x00000000
#define SR_FS_INITIAL   0x00002000
#define SR_FS_CLEAN     0x00004000
#define SR_FS_DIRTY     0x00006000

#define SR_XS           0x00018000 /* Extension Status */
#define SR_XS_OFF       0x00000000
#define SR_XS_INITIAL   0x00008000
#define SR_XS_CLEAN     0x00010000
#define SR_XS_DIRTY     0x00018000

#define SR_SD           0x8000000000000000 /* FS/XS dirty */

/* SATP flags 页表相关*/
#define SATP_PPN        0x00000FFFFFFFFFFF // 页表物理页号掩码
#define SATP_MODE_39    0x8000000000000000 // 39 位虚拟地址模式
#define SATP_MODE       SATP_MODE_39       // 当前默认模式

/* SCAUSE 异常和中断相关*/
#define SCAUSE_IRQ_FLAG   (1UL << 63) // SCAUSE 最高位为中断标志

#define IRQ_U_SOFT		0 // 用户软件中断
#define IRQ_S_SOFT		1 // S 特权级软件中断
#define IRQ_M_SOFT		3 // M 特权级软件中断
#define IRQ_U_TIMER		4 // 用户定时器中断
#define IRQ_S_TIMER		5 // S 特权级定时器中断
#define IRQ_M_TIMER		7 // M 特权级定时器中断
#define IRQ_U_EXT		8 // 用户外部中断
#define IRQ_S_EXT		9 // S 特权级外部中断
#define IRQ_M_EXT		11 // M 特权级外部中断

#define EXC_INST_MISALIGNED	0  // 指令未对齐异常
#define EXC_INST_ACCESS		1  // 指令访问异常
#define EXC_BREAKPOINT		3  // 断点异常
#define EXC_LOAD_ACCESS		5  // 读访问异常
#define EXC_STORE_ACCESS	7  // 写访问异常
#define EXC_SYSCALL		8  // 系统调用异常
#define EXC_INST_PAGE_FAULT	12 // 指令页错误
#define EXC_LOAD_PAGE_FAULT	13 // 读页错误
#define EXC_STORE_PAGE_FAULT	15 // 写页错误

/* SIE (Interrupt Enable) and SIP (Interrupt Pending) flags 中断使能/挂起相关*/
#define SIE_SSIE    (0x1 << IRQ_S_SOFT)   // S 软件中断使能
#define SIE_STIE    (0x1 << IRQ_S_TIMER)  // S 定时器中断使能
#define SIE_SEIE    (0x1 << IRQ_S_EXT)    // S 外部中断使能

/* 常用 CSR 寄存器编号 */
// 这些宏定义了常用的 CSR 编号，便于通过汇编或 C 代码访问相关状态寄存器
#define CSR_CYCLE   0xc00
#define CSR_TIME    0xc01
#define CSR_INSTRET   0xc02
#define CSR_SSTATUS   0x100
#define CSR_SIE     0x104
#define CSR_STVEC   0x105
#define CSR_SCOUNTEREN    0x106
#define CSR_SSCRATCH    0x140
#define CSR_SEPC    0x141
#define CSR_SCAUSE    0x142
#define CSR_STVAL   0x143
#define CSR_SIP     0x144
#define CSR_SATP    0x180
#define CSR_CYCLEH    0xc80
#define CSR_TIMEH   0xc81
#define CSR_INSTRETH    0xc82

#define CSR_MHARTID 0xf14

#endif /* CSR_H */
