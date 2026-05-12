#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define ONE_MB (1024 * 1024)
#define FILE_SIZE (128 * ONE_MB)
#define TEST_FILE "big.txt"

const char* begin_data = "First block data: Hello Direct Index!";
const char* end_data = "Last block data: Hello Indirect!";

int main(void)
{
    int fd;
    char write_buf[64] = {0};
    char read_buf[64] = {0};
    int nbytes;
    fd = sys_open(TEST_FILE, O_RDWR); 

    printf("--- Large File & Indirect Indexing Test ---\n");
    // 在文件开头写入数据 (直接索引块)
    strcpy(write_buf, begin_data);
    sys_write(fd, write_buf, strlen(write_buf));
    printf("[1] Written to the start of file.\n");

    // 定位到距离 128MB 结尾还剩 30 字节的地方
    int target_offset = FILE_SIZE - 30;
    int actual_pos = sys_lseek(fd, target_offset, SEEK_SET);
    printf("[2] Lseek to offset %d (SEEK_SET), actual pos: %d\n", target_offset, actual_pos);

    // 在末尾写入数据 (二级索引块)
    strcpy(write_buf, end_data);
    sys_write(fd, write_buf, strlen(write_buf));
    printf("[3] Written to the end of 128MB range.\n");

    // 重新定位到开头读取，验证直接索引
    sys_lseek(fd, 0, SEEK_SET);
    memset(read_buf, 0, sizeof(read_buf));
    sys_read(fd, read_buf, strlen(begin_data));
    printf("[4] Read from start: %s\n", read_buf);

    // 重新定位到末尾读取，验证间接索引映射的准确性
    sys_lseek(fd, target_offset, SEEK_SET);
    memset(read_buf, 0, sizeof(read_buf));
    sys_read(fd, read_buf, strlen(end_data));
    printf("[5] Read from 128MB offset: %s\n", read_buf);

    // 测试文件空洞
    sys_lseek(fd, 64 * ONE_MB, SEEK_SET); // 64MB 处
    nbytes = sys_read(fd, read_buf, 10);
    printf("[6] Read 10 bytes from 64MB hole, received %d bytes.\n", nbytes);

    int is_hole_empty = 1;
    for(int i = 0; i < 10; i++) if(read_buf[i] != 0) is_hole_empty = 0;
    if (is_hole_empty) printf("    Success: Hole contains only zeros.\n");
    sys_close(fd);
    printf("--- Test Finished ---\n");
    return 0;
}