#include <e1000.h>
#include <type.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/list.h>
#include <os/smp.h>
#include <os/irq.h>
#include <os/net.h>
#include <os/time.h>
#include <printk.h>
#include <screen.h>

static LIST_HEAD(reorder_queue);     // 乱序包链表头
// static int reorder_cnt = 0;          // 当前缓冲的乱序包数量
static uint32_t next_expect_seq = 0; // 下一个期望的序列号
stream_buffer_entry reorder_entries[MAX_REORDER_NUM]; 

extern list_head recv_block_queue;

// 用于保存最近一次收到的合法包的头部 (54字节)
static uint8_t last_valid_header[E1000_RTP_OFFSET];
static int has_valid_header = 0;

// === 字节序转换辅助函数 ===
static uint16_t ntohs(uint16_t v) {
    return (v << 8) | (v >> 8);
}
static uint16_t htons(uint16_t v) {
    return (v << 8) | (v >> 8);
}
static uint32_t ntohl(uint32_t v) {
    return (v << 24) | ((v & 0xff00) << 8) | ((v & 0xff0000) >> 8) | (v >> 24);
}
static uint32_t htonl(uint32_t v) {
    return (v << 24) | ((v & 0xff00) << 8) | ((v & 0xff0000) >> 8) | (v >> 24);
}

void init_stream(){
    init_queue(&reorder_queue);
    for (int i = 0; i < MAX_REORDER_NUM; i++) {
        reorder_entries[i].seq = 0;
        reorder_entries[i].len = 0;
    }
}

// 发送控制包（ACK 或 RSD）
static void send_control_packet(uint8_t flags, uint32_t seq) {
    // 分配一个缓冲区，报头大小
    uint8_t packet[MIN_PKG_SIZE]; 
    memset(packet, 0, MIN_PKG_SIZE);
    if (has_valid_header) {
        // 复制之前收到的头部 (Ethernet + IP + TCP)
        memcpy(packet, last_valid_header, E1000_RTP_OFFSET);

        screen_move_cursor(0, 4);
        printk("Client Mac: ");
        for (int i = 0; i < 6; i++) {
            printk("%2x ", packet[i]);
        }
        printk("\nHost Mac: ");
        for (int i = 0; i < 6; i++) {
            printk("%2x ", packet[i + 6]);
        }
        printk("\n");

        // 交换以太网 MAC 地址 (Dst <-> Src)
        // 以太网头前 6 字节是 Dst，第 6-11 字节是 Src
        uint8_t tmp_mac[6];
        memcpy(tmp_mac, packet, 6);
        memcpy(packet, packet + 6, 6);     
        memcpy(packet + 6, tmp_mac, 6);
        has_valid_header = 0;
        
    } else {
        // 如果还没收到过包，硬编码一个广播 MAC: FF:FF:FF:FF:FF:FF
        memset(packet, 0xFF, 6);
    }

    // 定位到 RTP 头的位置
    rtphdr *hdr = (rtphdr *)(packet + E1000_RTP_OFFSET);
    hdr->magic = RTP_MAGIC;
    hdr->flags = flags;
    hdr->len = 0;
    hdr->seq = htonl(seq);

    // 发送
    do_net_send(packet, MIN_PKG_SIZE);
}

// 辅助函数：获取重排序队列中seq的节点
stream_buffer_entry * get_reorder_entry(int seq){
    list_node_t *next = reorder_queue.next;
    while (next != &reorder_queue) {
        stream_buffer_entry *entry = CONTAINER_OF(next, stream_buffer_entry, list);
        if (entry->seq == seq) return entry;
        next = next->next; // 检查下一个结点
    }
    return NULL;
}

stream_buffer_entry * get_reorder_slot(){
    for (int i = 0; i < MAX_REORDER_NUM; i++) {
        stream_buffer_entry *entry = &reorder_entries[i];
        if (entry->len == 0) return entry;
    }
    return NULL;
}

void free_reorder_slot(list_node_t *node){
    stream_buffer_entry *entry = CONTAINER_OF(node, stream_buffer_entry, list);
    entry->seq = 0;
    entry->len = 0;
}


int compare_seq(list_node_t *node1, list_node_t *node2){
    stream_buffer_entry *entry1 = CONTAINER_OF(node1, stream_buffer_entry, list);
    stream_buffer_entry *entry2 = CONTAINER_OF(node2, stream_buffer_entry, list);
    return entry1->seq > entry2->seq;
}

// 将乱序包插入到重排序链表中 (按 Seq 从小到大排序)
static void insert_reorder_packet(uint32_t seq, int len, uint8_t *data) {
    if (seq < next_expect_seq) return; // 丢弃旧包
    if (seq > next_expect_seq + RTP_WINDOW_SIZE) return; // 丢弃窗口外的包
    if (get_reorder_entry(seq)) return; // 乱序包已存在
    
    // 分配新节点 
    stream_buffer_entry *new_entry = get_reorder_slot();
    if (!new_entry) return;

    new_entry->seq = seq;
    new_entry->len = len;
    memcpy(new_entry->data, data, len);

    // 插入链表
    list_insert(&new_entry->list, &reorder_queue, compare_seq);
    // reorder_cnt++;
}

void do_reset_stream(){
    next_expect_seq = 0; // 重置stream
}

int do_net_recv_stream(void *buffer, int *nbytes)
{
    int wanted = *nbytes; // 用户想要读取的总字节数
    int received = 0;     // 已经写入用户 buffer 的字节数
    // 临时缓冲区，用于接收底层网卡的原始数据包
    uint8_t raw_buffer[RX_PKT_SIZE]; 
    int pkt_len = 0; // 收取到的包长度
    uint64_t last_rsd_time = 0; // 记录上一次发送 RSD 的时间
    uint64_t last_progress_time = get_ticks(); // 上一次成功接收数据的时间

    while (received < wanted) {
        // 检查重排序缓冲区
        stream_buffer_entry *entry = get_reorder_entry(next_expect_seq);
        // 如果正好是期望的包
        if(entry){
            int copy_len = (entry->len > (wanted - received)) ? (wanted - received) : entry->len;
            memcpy((uint8_t *)buffer + received, entry->data, copy_len);
            received += copy_len;
            // ! 这里可能会导致重复发送 ACK，不过应该问题不大
            send_control_packet(RTP_ACK, next_expect_seq + entry->len);
            next_expect_seq += copy_len;
            last_progress_time = get_ticks(); // 有数据流就更新接收时间
            if (copy_len < entry->len) {
                // 数据没有完全被用户接受时，不需要删除缓冲项
                entry->len -= copy_len;
                entry->seq += copy_len;
            }else{
                del_node(&entry->list); // 删除节点
                free_reorder_slot(&entry->list); // 释放槽位
            }
            continue;

        }

        // 尝试从网卡接收新包
        // 使用 e1000_poll 进行非阻塞轮询
        pkt_len = e1000_poll(raw_buffer);
        if (pkt_len == 0) {
            // -- 没有收到包 --
            uint64_t current_time = get_ticks();
            // 超时检查：如果已经很久没有接收数据了，就退出
            if (current_time - last_progress_time > RECV_TIMEOUT_TICKS) {
                break; 
            }
            if (current_time - last_rsd_time > RSD_TIMEOUT_TICKS) {
                // 发送重传请求 next_expect_seq
                send_control_packet(RTP_RSD, next_expect_seq);
                last_rsd_time = current_time;
            }
            do_sleep(RSD_SLEEP_TIME);
            continue; // 醒来后继续循环尝试
        }

        // 处理收到的包
        if (pkt_len < E1000_RTP_OFFSET + sizeof(rtphdr)) continue; // 长度不对
        rtphdr *hdr = (rtphdr *)(raw_buffer + E1000_RTP_OFFSET);
        if (hdr->magic != RTP_MAGIC) continue; // 协议不对
        if (hdr->flags != RTP_DAT) continue; // 类型不对

        // TODO:只要收到符合协议的包，就先把它的头部保存下来
        // 这样发 ACK/RSD 时就有头部可以用了
        if (!has_valid_header) {
            memcpy(last_valid_header, raw_buffer, E1000_RTP_OFFSET);
            has_valid_header = 1;
        }

        // uint32_t seq = hdr->seq;
        // int len = hdr->len;
        // [修正] 接收时，将网络字节序 (Big Endian) 转为主机字节序
        uint32_t seq = ntohl(hdr->seq);
        int len = ntohs(hdr->len);

        uint8_t *data_ptr = raw_buffer + E1000_RTP_OFFSET + sizeof(rtphdr);
        if (seq == next_expect_seq) {
            // 顺序到达，直接拷贝给用户
            int copy_len = (len > (wanted - received)) ? (wanted - received) : len;
            memcpy((uint8_t *)buffer + received, data_ptr, copy_len);
            received += copy_len;
            send_control_packet(RTP_ACK, next_expect_seq + len);
            next_expect_seq += copy_len; // TODO: 下次期望从缓冲区读到
            if (copy_len < len) {
                // 数据没有完全被用户接受时，需要将剩余数据暂存起来
                insert_reorder_packet(next_expect_seq, len - copy_len, data_ptr + copy_len);
            }
            
        } else if (seq > next_expect_seq) {
            // 乱序到达，存入缓冲区
            insert_reorder_packet(seq, len, data_ptr);
            // 中间缺包，要求 RSD
            send_control_packet(RTP_RSD, next_expect_seq);
            last_rsd_time = get_ticks(); // 重置超时计时
        } else {
            // 旧包丢弃，发送 ACK
            send_control_packet(RTP_ACK, next_expect_seq);
        }
    }

    *nbytes = received; // 返回实际读取的字节数
    return received;
}