/*
 * asm.h: Assembler macros to make things easier to read.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 * Copyright (C) 1998 Ralf Baechle
 */
#ifndef ASM_H
#define ASM_H
// 定义函数的大小，.size 指令用于告诉汇编器该函数的长度。
#define END(function)  \
       .size function, .- function
// 定义一个过程（函数），标记其类型为函数类型，并设置其大小
#define ENDPROC(name)                           \
        .type name, @function;                  \
        END(name)
// 将符号导出为全局符号，方便其他文件引用
#define EXPORT(symbol) \
        .globl symbol; \
        symbol:
// 将函数符号导出为全局符号，并标记其类型为函数类型
#define FEXPORT(symbol)              \
        .globl symbol;               \
        .type symbol, @function;     \
        symbol:
// 定义一个入口点，确保其地址对齐到4字节边界，并将其标记为全局符号
#define ENTRY(name)                             \
        .globl name;                            \
        .balign 4;                              \
        name:
// 定义 RISC-V 架构下的指针大小和对齐方式
#define RISCV_PTR		.dword  // 指针类型为双字（8字节）
#define RISCV_SZPTR		8       // 指针大小为8字节
#define RISCV_LGPTR		3       // 指针大小的对数（log2(8) = 3）

#endif /* ASM_H */
