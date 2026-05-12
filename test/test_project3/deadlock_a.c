#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    int mbox1_idx = atoi(argv[1]); // MBOX1 (目标写入)
    int mbox2_idx = atoi(argv[2]); // MBOX2 (目标读取)
    long send_msg = 112233;
    int  recv_msg = 0;
    // char buffer_recv[10];
    
    // 先从 mbox2 读取数据
    printf("A: Attempting to receive from mbox2 (Index %d)...\n", mbox2_idx);
    sys_mbox_recv(mbox2_idx, &recv_msg, sizeof(int)); 
    
    // 后向 mbox1 发送数据
    printf("A: Received from mbox2! Now sending to mbox1 (Index %d)...\n", mbox1_idx);
    sys_mbox_send(mbox1_idx, &send_msg, sizeof(long));
    
    // 如果执行到这里，说明死锁被打破了
    printf("Process A: WARNING: Send succeeded unexpectedly!\n");
    return 0;
}