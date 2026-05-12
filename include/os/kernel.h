#ifndef __INCLUDE_KERNEL_H__
#define __INCLUDE_KERNEL_H__

#include <type.h>
#include <common.h>
#include <pgtable.h>
// 定义了一个**基础输入/输出系统（BIOS）的调用接口**，使得上层应用程序或操作系统代码能通过一个**跳转表（Jump Table）**机制来调用底层核心服务。
// ! 这个机制实现了**松耦合**，允许内核或固件在不修改调用代码的情况下更改底层服务的具体实现。
// **总结来说，这段代码的作用是建立了一套标准的、基于内存跳转表的底层服务调用规范，允许高层代码以简洁、统一的方式访问核心的控制台和存储设备驱动程序。**

// ### 1. 定义跳转表基址和索引
// KERNEL_JMPTAB_BASE定义了一个固定的内存地址，这是**内核服务跳转表的起始地址**。这个表存储了各个核心服务函数的实际地址。
// jmptab_idx_t 定义了一个枚举类型，作为**跳转表的索引**。每个成员（如 `CONSOLE_PUTSTR`、`SD_READ`）对应一个特定的底层服务，同时也代表了该服务函数指针在跳转表中的位置。

#define KERNEL_JMPTAB_BASE 0xffffffc051ffff00
typedef enum {
    CONSOLE_PUTSTR,
    CONSOLE_PUTCHAR,
    CONSOLE_GETCHAR,
    SD_READ,
    SD_WRITE,
    QEMU_LOGGING,
    SET_TIMER,
    READ_FDT,
    MOVE_CURSOR,
    WRITE,
    REFLUSH,
    PRINT,
    YIELD,
    MUTEX_INIT,
    MUTEX_ACQ,
    MUTEX_RELEASE,
    NUM_ENTRIES
} jmptab_idx_t; // unsigned long int = 4 Bytes 

// ### 2. 核心调用函数

// * **`static inline long call_jmptab(...)`**: 这是**实现核心功能**的内联函数。
//     1.  它根据传入的索引 `which`，计算出该服务函数指针在跳转表中的地址：
//         地址 = KERNEL_JMPTAB_BASE + sizeof(unsigned long) * which
//     2.  它从该地址**读取**出实际的函数指针 (`val`)。
//     3.  它将 `val` 转换为一个具有五个 `long` 参数和返回 `long` 值的函数指针，并使用传入的参数 (`arg0` 到 `arg4`) **执行**该函数。

static inline long call_jmptab(long which, long arg0, long arg1, long arg2, long arg3, long arg4)
{
    unsigned long val = \
        *(unsigned long *)(KERNEL_JMPTAB_BASE + sizeof(unsigned long) * which);
    long (*func)(long, long, long, long, long) = (long (*)(long, long, long, long, long))val;

    return func(arg0, arg1, arg2, arg3, arg4);
}
// ### 3. 封装的 BIOS 接口

// 该文件将 `call_jmptab` 进一步封装成一系列易于使用的 **BIOS (Basic Input/Output System) 风格**的函数：

// * **控制台 I/O**：`bios_putstr`、`bios_putchar`、`bios_getchar`，用于基础的字符和字符串输出/输入操作。
// * **SD 卡 I/O**：`bios_sd_read`、`bios_sd_write`，用于对 SD 卡进行块级别的读写操作。

static inline void bios_putstr(char *str)
{
    call_jmptab(CONSOLE_PUTSTR, (long)str, 0, 0, 0, 0);
}

static inline void bios_putchar(int ch)
{
    call_jmptab(CONSOLE_PUTCHAR, (long)ch, 0, 0, 0, 0);
}

static inline int bios_getchar(void)
{
    return call_jmptab(CONSOLE_GETCHAR, 0, 0, 0, 0, 0);
}

static inline int bios_sd_read(unsigned mem_address, unsigned num_of_blocks, \
                              unsigned block_id)
{
    return call_jmptab(SD_READ, (long)mem_address, (long)num_of_blocks, \
                        (long)block_id, 0, 0);
}

/************************************************************/

static inline int bios_sd_write(unsigned mem_address, unsigned num_of_blocks, \
                              unsigned block_id)
{
    return call_jmptab(SD_WRITE, (long)mem_address, (long)num_of_blocks, \
                        (long)block_id, 0, 0);
}

static inline void bios_logging(char *str)
{
    call_jmptab(QEMU_LOGGING, (long)str, 0, 0, 0, 0);
}

static inline void bios_set_timer(uint64_t stime_value)
{
    call_jmptab(SET_TIMER, (long)stime_value, 0, 0, 0, 0);
}

static inline uint64_t bios_read_fdt(enum FDT_TYPE type)
{
    return call_jmptab(READ_FDT, (long)type, 0, 0, 0, 0);
}
/************************************************************/

#endif