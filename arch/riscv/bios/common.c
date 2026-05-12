#include <common.h>
#include <asm/biosdef.h>

#define BIOS_FUNC_ENTRY 0xffffffc050150000
#define IGNORE 0

// 统一封装对 BIOS 的调用
// 通过函数指针调用 BIOS 固定入口，传递 8 个参数（其中最后一个是功能号 which，其余为实际参数或 IGNORE）。
static long call_bios(long which, long arg0, long arg1, long arg2, long arg3, long arg4)
{
    long (*bios_func)(long,long,long,long,long,long,long,long) = \
        (long (*)(long,long,long,long,long,long,long,long))BIOS_FUNC_ENTRY;

    return bios_func(arg0, arg1, arg2, arg3, arg4, IGNORE, IGNORE, which);
}
// 输出单个字符到端口
void port_write_ch(char ch)
{
    call_bios((long)BIOS_PUTCHAR, (long)ch, IGNORE, IGNORE, IGNORE, IGNORE);
}
// 输出字符串到端口
void port_write(char *str)
{
    call_bios((long)BIOS_PUTSTR, (long)str, IGNORE, IGNORE, IGNORE, IGNORE);
}
// 从端口读取单个字符
int port_read_ch(void)
{
    return call_bios((long)BIOS_GETCHAR, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}
// 从 SD 卡读取数据块到指定内存地址
int sd_read(unsigned mem_address, unsigned num_of_blocks, unsigned block_id)
{
    return (int)call_bios((long)BIOS_SDREAD, (long)mem_address, \
                            (long)num_of_blocks, (long)block_id, IGNORE, IGNORE);
}
// 从指定内存地址向 SD 卡写入数据块
int sd_write(unsigned mem_address, unsigned num_of_blocks, unsigned block_id)
{
    return (int)call_bios((long)BIOS_SDWRITE, (long)mem_address, \
                            (long)num_of_blocks, (long)block_id, IGNORE, IGNORE);
}

void set_timer(uint64_t stime_value)
{
    call_bios((long)BIOS_SETTIMER, (long)stime_value, IGNORE, IGNORE, IGNORE, IGNORE);
}

uint64_t read_fdt(enum FDT_TYPE type)
{
    return (uint64_t)call_bios((long)BIOS_FDTREAD, (long)type, IGNORE, IGNORE, IGNORE, IGNORE);
}

void qemu_logging(char *str)
{
    call_bios((long)BIOS_LOGGING, (long)str, IGNORE, IGNORE, IGNORE, IGNORE);
}

void send_ipi(const unsigned long *hart_mask)
{
    call_bios((long)BIOS_SEND_IPI, (long)hart_mask, IGNORE, IGNORE, IGNORE, IGNORE);
}