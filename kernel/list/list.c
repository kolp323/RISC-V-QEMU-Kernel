#include <os/list.h>
#include <os/sched.h>

void init_queue(list_head *head){
    head->next = head->prev = head;
}


// 是否空队(空：true)
int is_empty_queue(list_head *head)
{
    return head->next == head || \
           head->prev == head;
}


//入队
void enqueue(list_node_t *node, list_head *head)
{
    list_node_t *rear_ptr = head->prev;
    node->next = head;
    node->prev = rear_ptr;
    rear_ptr->next = node;
    head->prev = node;
}

//出队
list_node_t *dequeue(list_head *head)
{
    /* when empty, return NULL */
    if (is_empty_queue(head))
        return NULL;
    // 队首
    list_node_t *front = head->next;
    head->next = front->next;
    front->next->prev = head;
    front->next = front->prev = front;
    return front;
}

void del_node(list_node_t *node){
    list_node_t *prev = node->prev;
    list_node_t *next = node->next;
    prev->next = next;
    next->prev = prev;
    node->prev = node->next = node;
}

// 遍历队列，返回第一个满足条件的结点（删除），失败返回NULL
list_node_t *visit(list_head *head, int(*judge)(list_node_t *)){
    list_node_t *next = head->next;
    // 遍历队列一圈
    while (next != head) {
        if (judge(next)) {
            del_node(next);
            return next; // 满足条件
        }
        next = next->next; // 检查下一个结点
    }
    return NULL;
}

// 遍历队列，返回第一个满足条件的结点（不删除），失败返回NULL
list_node_t *visit_without_delete(list_head *head, int(*judge)(list_node_t *)){
    list_node_t *next = head->next;
    // 遍历队列一圈
    while (next != head) {
        if (judge(next)) {
            return next; // 满足条件
        }
        next = next->next; // 检查下一个结点
    }
    return NULL;
}

// 检查结点node是否在队列中
int is_in_queue(list_node_t* node, list_head *head){
    list_node_t *next = head->next;
    while (next != head) {
        if (node == next) {
            return 1; // 存在
        }
        next = next->next; // 检查下一个结点
    }
    return 0;
}

// 将 node 插入 head 队列中
// 将comp(n1,n2)>0时，将n1排在n2之后
void list_insert(list_node_t* node, list_head *head, int(*comp)(list_node_t *, list_node_t *)){
    list_node_t *pos = head->next; // 最终插在pos之前
    while (pos != head) {
        if (comp(node, pos) <= 0) break; // node < pos，插在pos之前
        pos = pos->next; // 检查下一个结点
    }
    list_node_t * prev = pos->prev;
    pos->prev = node;
    prev->next = node;
    node->prev = prev;
    node->next = pos;
}