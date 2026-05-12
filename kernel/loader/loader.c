#include <os/task.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/kernel.h>
#include <type.h>
#include <os/mm.h>
#include <printk.h>

uint64_t load_task_img(char *taskname, uintptr_t pgdir, int pid)
{
    /**
     * TODO:
     * 1. [p1-task3] load task from image via task id, and return its entrypoint
     * 2. [p1-task4] load task via task name, thus the arg should be 'char *taskname'
     */
    for (int i = 0; i < TASK_MAXNUM && strlen(taskname); i++) {
        if (strcmp(taskname, tasks[i].name) == 0) {
            // 获取程序信息
            uint64_t va_start = tasks[i].vaddr;
            uint64_t file_sz = tasks[i].filesz;
            uint64_t mem_sz = tasks[i].memsz;
            uint64_t image_offset = tasks[i].image_offset; // 程序在镜像中的字节偏移

            // 准备循环变量
            uint64_t current_va = va_start;
            uint64_t current_offset = image_offset; // 当前读取的镜像地址
            uint64_t remaining_filesz = file_sz; // 剩余的镜像文件长度
            uint64_t remaining_memsz = mem_sz;

            // 按页读取、拷贝、建立映射
            while (remaining_memsz > 0) {
                // 分配并映射一页
                uintptr_t page_kva = alloc_page_helper(current_va, pgdir, pid);
                // 计算当前页内偏移 (vaddr 可能不是页对齐的)
                uint64_t page_offset = current_va & (PAGE_SIZE - 1);
                uint64_t page_remain = PAGE_SIZE - page_offset; // 这一页剩余可用空间
                // 计算需要从文件拷贝多少数据：此页能直接拷贝完则全部拷贝，否则只拷贝可容纳的数据
                uint64_t copy_len = (remaining_filesz < page_remain) ? remaining_filesz : page_remain;
                if (copy_len > 0) {
                    // 从 SD 卡读取数据到 TEMP 缓冲区
                    // 计算起始扇区和扇区数
                    int start_sec = current_offset / SECTOR_SIZE;
                    int end_sec = (current_offset + copy_len - 1) / SECTOR_SIZE;
                    int num_secs = end_sec - start_sec + 1;
                    int offset_in_sec = current_offset % SECTOR_SIZE; // 第一个扇区内的偏移

                    // bios_sd_read(kva2pa(page_kva + page_offset - offset_in_sec), num_secs, start_sec);
                    // 读取数据到内核临时缓冲区 (TEMP 应该是 KVA)
                    bios_sd_read(kva2pa(TEMP), num_secs, start_sec);
                    // printl("DEBUG: TEMP[0] (KVA) = 0x%lx\n", *(uint64_t *)TEMP);
                    // 将数据拷贝到目标内存
                    memcpy((void *)(page_kva + page_offset), 
                           (void *)(TEMP + offset_in_sec), 
                           copy_len);
                    
                    // 更新偏移
                    current_offset += copy_len;
                    remaining_filesz -= copy_len;
                }

                // 清空 .bss 段
                // 如果这一页还有剩余空间，且文件数据已拷完，则填充 0
                if (copy_len < page_remain) {
                    memset((void *)(page_kva + page_offset + copy_len), 
                           0, 
                           page_remain - copy_len);
                }

                current_va += page_remain; // 增加此轮填充的数据长度
                // 更新占用内存大小
                remaining_memsz = (remaining_memsz > page_remain) ? (remaining_memsz - page_remain) : 0; 

            }

            return va_start;
        }
    }
     
    return 0; // 约定0入口点为无效程序
}