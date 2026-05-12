#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define MBOX_BUF_SIZE 64 // MAX_MBOX_LENGTH 是 64
#define MBOX_NAMELEN 10 

// 这是一个较大的占位消息，用来确保填满缓冲区
char buffer[MBOX_BUF_SIZE * 2] = {0}; 

int main() {
    int mbox1_idx, mbox2_idx;
    pid_t pid_a, pid_b;
    
    // 打开 Mailboxes
    mbox1_idx = sys_mbox_open("MBOX_A");
    mbox1_idx = sys_mbox_open("MBOX_A");
    mbox2_idx = sys_mbox_open("MBOX_B");
    mbox2_idx = sys_mbox_open("MBOX_B");

    printf("Deadlock Test Setup:\n");
    printf("MBOX_A Index: %d, MBOX_B Index: %d\n", mbox1_idx, mbox2_idx);

    // 先填满 Mailboxes 
    int fill_size = MBOX_BUF_SIZE; 
    
    sys_mbox_send(mbox1_idx, buffer, fill_size); // 填满 MBOX_A
    printf("MBOX_A filled to %d bytes.\n", fill_size);
    
    sys_mbox_send(mbox2_idx, buffer, fill_size); // 填满 MBOX_B
    printf("MBOX_B filled to %d bytes.\n", fill_size);
    
    // 准备参数
    char mbox1_str[MBOX_NAMELEN], mbox2_str[MBOX_NAMELEN];
    itoa(mbox1_idx, mbox1_str, MBOX_NAMELEN, 10);
    itoa(mbox2_idx, mbox2_str, MBOX_NAMELEN, 10);
    char *argv_a[] = {"deadlock_a", mbox1_str, mbox2_str, NULL};
    char *argv_b[] = {"deadlock_b", mbox1_str, mbox2_str, NULL};
    
    // 启动进程 A 和 B 
    pid_a = sys_exec(argv_a[0], 3, argv_a); 
    pid_b = sys_exec(argv_b[0], 3, argv_b); 
    printf("Deadlock: Starting Process A(pid:%d) and Process B(pid:%d) simultaneously.\n", pid_a, pid_b);
    // 留出时间让它们进入阻塞状态
    sys_sleep(1); 
    
    // 验证状态
    printf("Verification: Both processes should be BLOCKED (Deadlocked).\n");
    sys_ps(); 
    
    return 0;
}