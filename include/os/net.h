#ifndef __INCLUDE_NET_H__
#define __INCLUDE_NET_H__

#include <os/list.h>
#include <type.h>

#define PKT_NUM 32

#define ETH_ALEN 6u                 // Length of MAC address
#define ETH_P_IP 0x0800u            // IP protocol
// Ethernet header
struct ethhdr {
    uint8_t ether_dmac[ETH_ALEN];   // destination mac address
    uint8_t ether_smac[ETH_ALEN];   // source mac address
    uint16_t ether_type;            // protocol format
};

void net_handle_irq(void);
int do_net_recv(void *rxbuffer, int pkt_num, int *pkt_lens);
int do_net_send(void *txpacket, int length);
void init_net();


// [p5-task4]
#define E1000_RTP_OFFSET 54 // RTP协议从第 55 个字节开始
#define RTP_MAGIC 0x45
#define RTP_DAT 0x01 
#define RTP_RSD 0x02  
#define RTP_ACK 0x04  

#define HEAD_SIZE (E1000_RTP_OFFSET + 8) // 报头大小：TCP+IP+RTP
#define MIN_PKG_SIZE 64

// TODO: 在 pktRxTx 命令行里设置的窗口大小
#define RTP_WINDOW_SIZE 4096
// 乱序缓冲区的大小，稍微比窗口大一点点
#define MAX_REORDER_BUF (RTP_WINDOW_SIZE + 4)
#define MAX_REORDER_NUM 32 // 暂存乱序包的最大数量
#define RSD_TIMEOUT_TICKS 100000 // RSD 重传超时时间
#define RECV_TIMEOUT_TICKS (RSD_TIMEOUT_TICKS * 30) // 读取超时时间
#define RSD_SLEEP_TIME 1

struct rtphdr {
    uint8_t magic;
    uint8_t flags;
    uint16_t len;
    uint32_t seq;
} __attribute__((packed));
typedef struct rtphdr rtphdr;

// 乱序包结构体
struct stream_buffer_entry {
    list_node_t list;
    uint32_t seq;
    int len;
    uint8_t data[2048]; // 存储完整的数据载荷
};
typedef struct stream_buffer_entry stream_buffer_entry;

int do_net_recv_stream(void *buffer, int *nbytes);
void do_reset_stream();
void init_stream();
#endif  // __INCLUDE_NET_H__
