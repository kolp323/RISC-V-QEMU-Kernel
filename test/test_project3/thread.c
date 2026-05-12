// deadlock_fix_driver.c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "thread_utils.h"

#define MBOX_BUF_SIZE 64 // MAX_MBOX_LENGTH 是 64
#define MBOX_NAMELEN 10 

// 这是一个较大的占位消息，用来确保填满缓冲区
char buffer[MBOX_BUF_SIZE * 2]; 
int main() {
    int mbox1_idx, mbox2_idx;
    pid_t pid_a, pid_b;
    // 屏幕定位参数
    int driver_pos = 6;
    int pos_a = 7;
    int pos_b = 10;
    
    // 初始化屏幕显示
    sys_move_cursor(0, driver_pos);
    printf("[Driver] Starting Deadlock Avoidance Test...");
    
    // 提前打开 Mailboxes
    mbox1_idx = sys_mbox_open("MBOX1");
    mbox1_idx = sys_mbox_open("MBOX1");
    mbox2_idx = sys_mbox_open("MBOX2");
    mbox2_idx = sys_mbox_open("MBOX2");
    
    sys_move_cursor(0, driver_pos);
    printf("[Driver] Launching processes...               ");

    // 准备参数 (Process A/B 需要 MBOX1, MBOX2 索引和角色)
    char mbox1_str[MBOX_NAMELEN], mbox2_str[MBOX_NAMELEN];
    char pos_a_str[MBOX_NAMELEN], pos_b_str[MBOX_NAMELEN];
    itoa(mbox1_idx, mbox1_str, MBOX_NAMELEN, 10);
    itoa(mbox2_idx, mbox2_str, MBOX_NAMELEN, 10);
    itoa(pos_a, pos_a_str, MBOX_NAMELEN, 10); // 传递打印位置
    itoa(pos_b, pos_b_str, MBOX_NAMELEN, 10);

    char *argv_a[] = {"thread_template", mbox1_str, mbox2_str, "A", pos_a_str, NULL};
    char *argv_b[] = {"thread_template", mbox1_str, mbox2_str, "B", pos_b_str, NULL};
    // 启动进程 A 和 B
    pid_a = sys_exec(argv_a[0], 5, argv_a); 
    pid_b = sys_exec(argv_b[0], 5, argv_b);
    
    // 等待进程完成 (验证修复是否成功，程序是否能正常退出)
    sys_waitpid(pid_a);
    sys_waitpid(pid_b);
    
    sys_move_cursor(0, driver_pos);
    printf("Driver: Processes A and B completed successfully (Deadlock Avoided).\n");
    
    // 清理 Mailboxes
    sys_mbox_close(mbox1_idx);
    sys_mbox_close(mbox1_idx);
    sys_mbox_close(mbox2_idx);
    sys_mbox_close(mbox2_idx);
    
    return 0;
}