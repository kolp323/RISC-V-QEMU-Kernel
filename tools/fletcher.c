#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_RECV_SIZE (10 * 1024)

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
    if (argc > 1) {
        char *filename = argv[1];
        printf("\n[TEST] Validating against local file: %s ...\n", filename);
        
        FILE *fp = fopen(filename, "rb"); // 以二进制只读方式打开
        if (fp == NULL) {
            printf("[TEST] Error: Cannot open file '%s'\n", filename);
        } else {
            // 获取文件大小
            fseek(fp, 0, SEEK_END);
            long local_fsize = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            // 分配缓冲区读取文件
            uint8_t local_buf[MAX_RECV_SIZE];

            fread(local_buf, 1, local_fsize, fp);
            fclose(fp);

            // 计算本地文件的校验和
            uint16_t local_checksum = fletcher16(local_buf, local_fsize);
            
            printf("[TEST] Local File Size: %ld bytes\n", local_fsize);
            printf("[TEST] Local Checksum:  0x%04x\n", local_checksum);

        }
    } else {
        printf("\n[TEST] No local file specified for validation. (Usage: recv_stream <filename>)\n");
    }
}