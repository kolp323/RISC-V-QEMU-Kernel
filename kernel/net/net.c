#include <e1000.h>
#include <type.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/list.h>
#include <os/smp.h>
#include <os/irq.h>
#include <os/net.h>

static LIST_HEAD(send_block_queue);
static LIST_HEAD(recv_block_queue);

void init_net(){
    init_queue(&send_block_queue);
    init_queue(&recv_block_queue);
    init_stream();
}


int do_net_send(void *txpacket, int length)
{
    // TODO: [p5-task1] Transmit one network packet via e1000 device
    int bytes_sent = 0;
    while (1) {
        // 调用底层驱动发送
        bytes_sent = e1000_transmit(txpacket, length);
        // 如果返回值大于 0，说明发送成功，跳出循环
        if (bytes_sent > 0) break;  
        disable_preempt();
        // 开发送中断
        e1000_write_reg(e1000, E1000_IMS, E1000_IMS_TXQE);
        local_flush_dcache();
        do_block(&current_running->list, &send_block_queue);      
        enable_preempt();
        do_scheduler();
    }
    // TODO: [p5-task3] Call do_block when e1000 transmit queue is full
    // TODO: [p5-task4] Enable TXQE interrupt if transmit queue is full

    return bytes_sent;  // Bytes it has transmitted
}

int do_net_recv(void *rxbuffer, int pkt_num, int *pkt_lens)
{
    // TODO: [p5-task2] Receive one network packet via e1000 device
    int total_len = 0;      // 累计接收的总字节数
    int current_len = 0;    // 当前包的长度
    char *curr_buf = (char *)rxbuffer;

    for (int i = 0; i < pkt_num; i++) {
        while (1) {
            // 尝试从网卡获取数据
            current_len = e1000_poll((void *)curr_buf);
            if (current_len) {
                pkt_lens[i] = current_len; // 记录该包长度
                curr_buf += current_len; // 指针后移，为下一个包腾出位置
                total_len += current_len; // 累加总长度
                break;
            }
            disable_preempt();
            // 打开接收中断
            e1000_write_reg(e1000, E1000_IMS, E1000_IMS_RXDMT0);
            local_flush_dcache();
            do_block(&current_running->list, &recv_block_queue);
            enable_preempt();
            do_scheduler();
        }
    }

    // TODO: [p5-task3] Call do_block when there is no packet on the way

    return total_len;  // Bytes it has received
}

// E1000 TXQE(Transmit Queue Empty)发送中断
void e1000_handle_txqe(void){
    // 发送队列有空位了，唤醒 sender
    wakeup(&send_block_queue);
    e1000_write_reg(e1000, E1000_IMC, E1000_IMC_TXQE);
    local_flush_dcache();
}
// E1000 RXDMT0(Receive Descriptor Min Threshold)接收中断
void e1000_handle_rxdmt0(void){
    // 有数据包到达（描述符被消耗），唤醒 recver
    wakeup(&recv_block_queue);
    e1000_write_reg(e1000, E1000_IMC, E1000_IMC_RXDMT0);
    local_flush_dcache();
}
void net_handle_irq(void)
{
    // TODO: [p5-task3] Handle interrupts from network device
    // 读取 ICR 寄存器，读取后硬件会自动清零
    local_flush_dcache();
    uint32_t icr = e1000_read_reg(e1000, E1000_ICR);
    if(icr & E1000_ICR_TXQE) e1000_handle_txqe();
    if(icr & E1000_ICR_RXDMT0) e1000_handle_rxdmt0();

}
