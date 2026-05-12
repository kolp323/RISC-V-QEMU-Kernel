#include <asm/biosdef.h>
#include <csr.h>

// os size location (os_size could be sectors [p1-task3] or bytes [p1-task4])
.equ os_size_loc, 0x502001fc

// kernel address (move kernel to here, and jmp here to start kernel)
.equ kernel, 0x50202000

// BIOS function entry (jump here to use BIOS APIs)
.equ bios_func_entry, 0x50150000

.text
.global main

main:
	// fence on all memory and I/O
	fence
    // a0 is mhartid
	bnez a0, secondary

	// TODO: [p1-task1] call BIOS to print string "It's bootblock!"
	# void port_write(char *str)
	li a7, BIOS_PUTSTR 			// BIOS call: print string
	la a0, msg        			// load address of string
	jal bios_func_entry

	// TODO: [p1-task2] call BIOS to read kernel in SD card
	# int sd_read(unsigned mem_address, unsigned num_of_blocks, unsigned block_id)
	li a7, BIOS_SDREAD			// BIOS call: read from SD card
	la a0, kernel    			// load memory address to store the kernel
	la t0, os_size_loc			// t0 = 0x502001fc
	lh a1, 0(t0)				// a1 = *0x502001fc 内核占了几个扇区的信息，会写在第一个扇区的倒数第 4 个字节的位置（0x502001fc），长度为 2 字节
	li a2, 1					// load block ID to read (block 0 is MBR, block 1 is the first block of kernel)
	jal bios_func_entry

	// TODO: [p1-task4] load task-related arguments and pass them to kernel

	
	// TODO: [p1-task2] jump to kernel to start UCAS-OS
	j kernel

	
	/************************************************************/
secondary:
	/* TODO [P3-task3]: */
	// 1. Mask all interrupts
	// CSR_SIE (Supervisor Interrupt Enable Register) 用于控制哪些中断可以被 S 模式接收。
	// CSR_SIP (Supervisor Interrupt Pending Register) 用于显示哪些中断已经被设置为待处理状态（Pending）。
	// 屏蔽 S 模式下的所有中断。清除所有待处理的中断标志，防止在中断被启用后立即触发任何旧的中断。
	csrw CSR_SIE, zero
	csrw CSR_SIP, zero
	// 2. let stvec pointer to kernel_main
	la t0, kernel
	csrw stvec, t0
	// 3. enable software interrupt for ipi
	li t0, SIE_SSIE  // S 模式软件中断启用位（SSIE，Supervisor Software Interrupt Enable）在 sie 寄存器中的位掩码
    csrs sie, t0 // 设置 sie 寄存器的 SSIE 位（置 1）
	li t0, SR_SIE // S 模式全局中断启用位（SIE，Supervisor Interrupt Enable）在 sstatus 寄存器中的位掩码
	csrs CSR_SSTATUS, t0 // 全局性地打开了S模式的中断
	
wait_for_wakeup:
	wfi
	j wait_for_wakeup

	/************************************************************/
// while(1) --> stop here
stop:
	j stop
	nop

.data
msg: .string "It's zhushouhe's bootloader...\n\r"

