#include <e1000.h>
#include <type.h>
#include <os/string.h>
#include <os/time.h>
#include <assert.h>
#include <pgtable.h>

// E1000 Registers Base Pointer
volatile uint8_t *e1000;  // use virtual memory address

// E1000 Tx & Rx Descriptors
static struct e1000_tx_desc tx_desc_array[TXDESCS] __attribute__((aligned(16)));
static struct e1000_rx_desc rx_desc_array[RXDESCS] __attribute__((aligned(16)));

// E1000 Tx & Rx packet buffer
static char tx_pkt_buffer[TXDESCS][TX_PKT_SIZE];
static char rx_pkt_buffer[RXDESCS][RX_PKT_SIZE];

// Fixed Ethernet MAC Address of E1000
static const uint8_t enetaddr[6] = {0x00, 0x0a, 0x35, 0x00, 0x1e, 0x53};

/**
 * e1000_reset - Reset Tx and Rx Units; mask and clear all interrupts.
 **/
static void e1000_reset(void)
{
	/* Turn off the ethernet interface */
    e1000_write_reg(e1000, E1000_RCTL, 0);
    e1000_write_reg(e1000, E1000_TCTL, 0);

	/* Clear the transmit ring */
    e1000_write_reg(e1000, E1000_TDH, 0);
    e1000_write_reg(e1000, E1000_TDT, 0);

	/* Clear the receive ring */
    e1000_write_reg(e1000, E1000_RDH, 0);
    e1000_write_reg(e1000, E1000_RDT, 0);

	/**
     * Delay to allow any outstanding PCI transactions to complete before
	 * resetting the device
	 */
    latency(1);

	/* Clear interrupt mask to stop board from generating interrupts */
    e1000_write_reg(e1000, E1000_IMC, 0xffffffff);

    /* Clear any pending interrupt events. */
    while (0 != e1000_read_reg(e1000, E1000_ICR)) ;
}

/**
 * e1000_configure_tx - Configure 8254x Transmit Unit after Reset
 **/
static void e1000_configure_tx(void)
{
    /* TODO: [p5-task1] Initialize tx descriptors */
    // 清零发送描述符数组
    memset(tx_desc_array, 0, sizeof(e1000_tx_desc) * TXDESCS);

    /* TODO: [p5-task1] Set up the Tx descriptor base address and length */
    // 获取发送描述符数组的物理地址
    uintptr_t tx_desc_pa = kva2pa((uintptr_t)tx_desc_array);
    // 写入 TDBAL (低32位) 和 TDBAH (高32位)
    e1000_write_reg(e1000, E1000_TDBAL, tx_desc_pa & 0xffffffff);
    e1000_write_reg(e1000, E1000_TDBAH, tx_desc_pa >> 32);
    // 写入 TDLEN (存储循环队列所占字节数)
    e1000_write_reg(e1000, E1000_TDLEN, TXDESCS * sizeof(e1000_tx_desc));

	/* TODO: [p5-task1] Set up the HW Tx Head and Tail descriptor pointers */
    // 初始化 Head 和 Tail 指针为 0 （描述符元素的索引）
    e1000_write_reg(e1000, E1000_TDH, 0);
    e1000_write_reg(e1000, E1000_TDT, 0);

    /* TODO: [p5-task1] Program the Transmit Control Register */
    // 设置 TCTL 寄存器
    // 设置 TCTL.EN 为 1、TCTL.PSP 为 1、
    // TCTL.CT 为0x10H、TCTL.COLD 为 0x40H
    uint32_t tctl = 0;
    tctl |= E1000_TCTL_EN;
    tctl |= E1000_TCTL_PSP;
    tctl |= (0x10 << 4);   // CT 0x00000ff0
    tctl |= (0x40 << 12);  // COLD 0x003ff000
    e1000_write_reg(e1000, E1000_TCTL, tctl);
    
    local_flush_dcache();
}

/**
 * e1000_configure_rx - Configure 8254x Receive Unit after Reset
 **/
static void e1000_configure_rx(void)
{
    /* TODO: [p5-task2] Set e1000 MAC Address to RAR[0] */
    // 设置 MAC 地址 (RAL0 / RAH0) 6 字节
    uint32_t ral = *(uint32_t *)(enetaddr); // 低4字节
    uint32_t rah = *(uint16_t *)(enetaddr + 4); // 高2字节
    rah |= E1000_RAH_AV; // 设置 RAH0.AV (Address Valid) 为 1
    // 写入数组寄存器 RA (Receive Address)：offset 0 -> RAL0, offset 1 -> RAH0
    e1000_write_reg_array(e1000, E1000_RA, 0, ral);
    e1000_write_reg_array(e1000, E1000_RA, 1, rah);

    /* TODO: [p5-task2] Initialize rx descriptors */
    for (int i = 0; i < RXDESCS; i++) {
        rx_desc_array[i].status = 0;
        rx_desc_array[i].length = 0;
        rx_desc_array[i].errors = 0;
        rx_desc_array[i].csum = 0;

        rx_desc_array[i].addr = kva2pa((uintptr_t)rx_pkt_buffer[i]);
    }

    /* TODO: [p5-task2] Set up the Rx descriptor base address and length */
    // 描述符循环数组基址
    uintptr_t rx_desc_pa = kva2pa((uintptr_t)rx_desc_array);
    e1000_write_reg(e1000, E1000_RDBAL, rx_desc_pa & 0xffffffff);
    e1000_write_reg(e1000, E1000_RDBAH, rx_desc_pa >> 32);
    // 存储循环队列所占字节数
    e1000_write_reg(e1000, E1000_RDLEN, RXDESCS * sizeof(e1000_rx_desc));

    /* TODO: [p5-task2] Set up the HW Rx Head and Tail descriptor pointers */
    e1000_write_reg(e1000, E1000_RDH, 0); // 指向硬件应该填入的描述符
    e1000_write_reg(e1000, E1000_RDT, RXDESCS - 1); // 指向软件刚读取完的描述符

    /* TODO: [p5-task2] Program the Receive Control Register */
    // RCTL.EN 为 1、RCTL.BAM 为 1、
    // RCTL.BSEX 为0、RCTL.BSIZE 为 0
    uint32_t rctl = (E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SZ_2048) & ~E1000_RCTL_BSEX;
    e1000_write_reg(e1000, E1000_RCTL, rctl);

    /* TODO: [p5-task4] Enable RXDMT0 Interrupt */

    local_flush_dcache();
}

/**
 * e1000_init - Initialize e1000 device and descriptors
 **/
void e1000_init(void)
{
    /* Reset E1000 Tx & Rx Units; mask & clear all interrupts */
    e1000_reset();

    /* Configure E1000 Tx Unit */
    e1000_configure_tx();

    /* Configure E1000 Rx Unit */
    e1000_configure_rx();
}

/**
 * e1000_transmit - Transmit packet through e1000 net device
 * @param txpacket - The buffer address of packet to be transmitted
 * @param length - Length of this packet
 * @return - Number of bytes that are transmitted successfully
 **/
int e1000_transmit(void *txpacket, int length)
{
    local_flush_dcache();
    /* TODO: [p5-task1] Transmit one packet from txpacket */
    // tail 指向下一个空闲的描述符槽位
    uint32_t tail = e1000_read_reg(e1000, E1000_TDT);
    uint32_t head = e1000_read_reg(e1000, E1000_TDH);
    
    // 队列满，返回 0，告诉上层需要等待
    if (head == (tail + 1) % TXDESCS) return 0; 

    e1000_tx_desc *desc = &tx_desc_array[tail];
    // 将数据拷贝到驱动内部的专用缓冲区
    if (length > TX_PKT_SIZE) {
        length = TX_PKT_SIZE; // 截断防止溢出
    }
    memcpy((uint8_t *)tx_pkt_buffer[tail], txpacket, length);

    // 填充发送描述符
    desc->addr = kva2pa((uintptr_t)tx_pkt_buffer[tail]); // 数据缓冲区物理地址
    desc->length = length;
    // CMD 字段设置：
    // E1000_TXD_CMD_DEXT: 0 (描述符为 Legacy 布局)
    // E1000_TXD_CMD_RS:  Report Status，要求网卡发完后记录传输的状态于描述符的 STA 字段
    // E1000_TXD_CMD_EOP: End Of Packet，表示当前数据帧是数据包的最后一个帧
    desc->cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS;

    desc->status = 0; // 清除状态位，避免干扰
    // 清空其他未使用字段
    desc->css = 0;
    desc->cso = 0;
    desc->special = 0; 

    // 确保 DMA 能读到描述符和缓冲区的数据
    local_flush_dcache(); 

    // 更新 TDT，触发网卡开始工作
    e1000_write_reg(e1000, E1000_TDT, (tail + 1) % TXDESCS);

    local_flush_dcache();
    return length;
}

/**
 * e1000_poll - Receive packet through e1000 net device
 * @param rxbuffer - The address of buffer to store received packet
 * @return - Length of received packet
 **/
int e1000_poll(void *rxbuffer)
{
    local_flush_dcache();
    /* TODO: [p5-task2] Receive one packet and put it into rxbuffer */
    uint32_t tail = e1000_read_reg(e1000, E1000_RDT); // 指向软件刚读取完的描述符
    uint32_t next = (tail + 1) % RXDESCS; // 尝试读取 tail 的下一个描述符数据
    e1000_rx_desc *desc = &rx_desc_array[next];
    // 如果 DD 位为 0，说明硬件还没有完成对该描述符的写入
    if ((desc->status & E1000_RXD_STAT_DD) == 0) return 0;

    // 有数据可读时
    // 刷新 D-Cache，确保 CPU 读到的是内存中 DMA 写进去的新数据
    uint16_t length = desc->length;
    if (rxbuffer) memcpy(rxbuffer, (uint8_t *)rx_pkt_buffer[next], length);

    // 重置描述符状态
    desc->status = 0;
    desc->length = 0;
    desc->csum = 0; 
    desc->errors = 0;

    e1000_write_reg(e1000, E1000_RDT, next);
    local_flush_dcache();
    return length;
}