#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define FILE_DATA_OFFSET 4

// 最大的接收缓冲区大小 (10kB)
#define MAX_RECV_SIZE (10 * 1024)
static char buffer[MAX_RECV_SIZE];

uint16_t fletcher16(uint8_t *data, int n) {
    uint16_t sum1 = 0;
    uint16_t sum2 = 0;
    int i;
    for (i = 0; i < n; ++i) {
        sum1 = (sum1 + data[i]) % 0xff;
        sum2 = (sum2 + sum1) % 0xff;
    }
    return (sum2 << 8) | sum1;
}

int main(int argc, char *argv[])
{
    memset(buffer, 0, MAX_RECV_SIZE);
    int print_location = 1;
    sys_move_cursor(0, print_location);
    printf("[RECV] Starting stream receiver...\n");

    sys_reset_stream(); // 初始化文件接收流
    int total_received = 0;
    int file_size = 0;
    int nbytes = 0;
    int ret = 0;

    // 接收第一个包（包含文件大小头）
    // 先尝试读取一小部分，至少要读到前4个字节
    int header_read_size = 64; 
    while (total_received < FILE_DATA_OFFSET) {
        nbytes = header_read_size; 
        ret = sys_net_recv_stream(buffer + total_received, &nbytes);
        
        if (ret > 0) {
            total_received += ret;
        }
    }

    // 解析文件大小
    // 前4个字节是 int 类型的 size (应用层协议) = 文件大小 + 4
    file_size = *(int *)buffer;
    file_size -= FILE_DATA_OFFSET; // 真实的文件大小
    sys_move_cursor(0, print_location);

    // 校验大小是否合理
    if (file_size <= 0 || file_size > MAX_RECV_SIZE - FILE_DATA_OFFSET) {
        printf("[Warn] Fallback to max buffer size: %d\n", file_size);
        // 如果不是文件传输模式（如自动生成数据），可能没有这4字节头
        // 可以选择继续接收直到填满 buffer
        file_size = MAX_RECV_SIZE - FILE_DATA_OFFSET; 
    }else{
        printf("[RECV] Detected file size: %d bytes\n", file_size);
    }

    // 循环接收剩余数据
    // 目标总接收量 = 4字节头 + 文件内容
    int target_total = FILE_DATA_OFFSET + file_size;
    
    // 用于打印进度的计数器
    int last_print = 0;

    while (total_received < target_total) {
        // 期望接收剩余所有
        nbytes = target_total - total_received;
        
        // 调用内核接收
        ret = sys_net_recv_stream(buffer + total_received, &nbytes);

        if (ret > 0) {
            total_received += ret;
            
            // 每接收 1KB 打印一次进度
            if (total_received - last_print > 1024) {
                sys_move_cursor(0, print_location + 1);
                printf("[RECV] Got %d / %d bytes\n", total_received, target_total);
                last_print = total_received;
            }
        } else {
            // 暂时没数据，等待
            sys_yield();
        }
    }
    sys_move_cursor(0, print_location + 1);
    printf("[RECV] Transfer Complete! Total received: %d bytes\n", total_received);

    // 校验与验证
    uint8_t *file_data = (uint8_t *)(buffer + FILE_DATA_OFFSET);
    
    // 计算校验和
    uint16_t checksum = fletcher16(file_data, file_size);
    printf("[RECV] Fletcher16 Checksum: 0x%04x\n", checksum);
    
    // 如果是文本文件，可以尝试打印一下字符串
    printf("--- Content ---\n%s\n", file_data);
    return 0;
}