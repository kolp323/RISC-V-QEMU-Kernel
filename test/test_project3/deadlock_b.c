#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    int mbox1_idx = atoi(argv[1]); // MBOX1 (目标读取)
    int mbox2_idx = atoi(argv[2]); // MBOX2 (目标写入)
    long send_msg = 112233;
    int  recv_msg = 0;

    // 先从 mbox1 读取数据 (会阻塞，因为 mbox1 为空)
    printf("B: Attempting to receive from mbox1 (Index %d)...\n", mbox1_idx);
    sys_mbox_recv(mbox1_idx, &recv_msg, sizeof(int)); // 应该在这里阻塞！
    
    // 后向 mbox2 发送数据
    printf("B: Received from mbox1! Now sending to mbox2 (Index %d)...\n", mbox2_idx);
    sys_mbox_send(mbox2_idx, &send_msg, sizeof(long));
    
    // 如果执行到这里，说明死锁被打破了
    printf("Process B: WARNING: Send succeeded unexpectedly!\n");
    return 0;
}