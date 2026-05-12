#include <os/string.h>
#include <os/task.h>
#include <os/sched.h>
#include <os/pipe.h>
#include <common.h>
#include <os/kernel.h>
#include <printk.h>
#include <os/loader.h>

int batch_offset_img;
short tasknum;
int taskinfo_offset_img;
task_info_t tasks[TASK_MAXNUM];
load_history_t load_history[TASK_MAXNUM];

const char *list = "list";
const char *wb = "wb";
const char *quit_batch = "quit";
const char *eb = "eb";

// 返回name对应的任务信息
// 未找到，返回NULL
// 若为空串，返回NULL
task_info_t *get_task_info(char *name){
    for (int i = 0; i < TASK_MAXNUM; i++) {
        if (strcmp(tasks[i].name, name) == 0 && strlen(name)) {
            return &tasks[i];
        }
    }
    return NULL;
}


void init_history(void){
    for (int i = 0; i < TASK_MAXNUM; i++) {
        load_history[i].is_loaded = 0;
        load_history[i].entrypoint = 0;
    }
}

// 任务相关函数
// 从sd卡中加载任务信息
void init_task_info(void)
{
    // TODO: [p1-task4] Init 'tasks' array via reading app-info sector
    // NOTE: You need to get some related arguments from bootblock first
    // 1. 从 bootblock 尾部读取元数据
    short *tasknum_ptr = (short *)(BOOTBLOCK_BASE + TASK_NUM_LOC);
    tasknum = *tasknum_ptr;
    int *taskinfo_offset_ptr = (int *)(BOOTBLOCK_BASE + TASKINFO_OFFSET_LOC);
    taskinfo_offset_img = *taskinfo_offset_ptr;

    // 2. 计算 taskinfo 加载参数
    unsigned start_block = taskinfo_offset_img / SECTOR_SIZE;
    int total_bytes = TASK_MAXNUM * sizeof(task_info_t);
    unsigned end_block = (taskinfo_offset_img + total_bytes - 1) / SECTOR_SIZE;
    unsigned num_block = end_block - start_block + 1;
    int offset_inblock = taskinfo_offset_img % SECTOR_SIZE; // 首块内偏移（因为没有扇区对齐）


    // 3. 读取 taskinfo 到 Kernel 内存中
    bios_sd_read(
        (unsigned int)kva2pa(TEMP),                
        num_block,   
        start_block
    );
    memcpy(
        (uint8_t *)tasks, 
        (uint8_t *)TEMP+offset_inblock, 
        (uint32_t)total_bytes
    );

    // 5. 读取batch sequence的位置 batch_offset_img
    int *batch_offset_ptr = (int *)(BOOTBLOCK_BASE + BATCH_OFFSET_LOC);
    batch_offset_img = *batch_offset_ptr;
}


void list_tasks(void){
    // bios_putstr("Available tasks include:\n\r");
    // for (int i = 0; i < tasknum; i++) {
    //     bios_putstr(COLOR_BLUE);
    //     bios_putstr(tasks[i].name);
    //     bios_putstr("\n");
    //     bios_putstr(COLOR_RESET);
    // }
    printk("Available tasks include:\n");
    for (int i = 0; i < tasknum; i++) {
        // printk(COLOR_BLUE);
        printk(tasks[i].name);
        printk("\n");
        // printk(COLOR_RESET);
    }
}

// TODO: p1-task5 批处理-----------------------------------------
//------执行APP
void exec_app(uint64_t entry_point, int is_batch){
        // 约定0入口点为无效程序
        if (entry_point != 0) {
            if(is_batch == SINGLE){
                bios_putstr(COLOR_GREEN);
                bios_putstr("Launching App...\n\r");
                bios_putstr(COLOR_RESET);
            }
            void (*entry)(void) = (void (*)(void))(entry_point);
            entry();
        } else {
            // 错误处理：App 加载失败或 ID 无效
            bios_putstr(COLOR_YELLOW);
            bios_putstr("[WARNING]:Invalid task name or failed to load app.\n\r");
            bios_putstr(COLOR_RESET);
        }
}

int is_valid_batch(char *buffer, int len){
    char *p = buffer;
    char *task_name = p; // 当前任务名的起始位置
    while (1) {
        if(*p == '\0') break; // 批处理序列结束
        // 每找到一个空格，尝试解析一个任务名
        if(*p == ' '){
            *p = '\0'; // 将空格替换为字符串结束符
            // 检查 task_name 是否在 tasks 中
            int found = 0;
            for (int i = 0; i < tasknum; i++) {
                if (strcmp(task_name, tasks[i].name) == 0) {
                    found = 1;
                    break;
                }
            }
            if (!found) return 0; // 未找到任务，批处理序列无效
            *p = ' '; // 恢复空格
            p++;
            task_name = p; // 更新下一个任务名的起始位置
            continue;
        }
        p++;
    }
    // 检查最后一个任务名
    if (task_name != p) { // 确保不是空字符串
        int found = 0;
        for (int i = 0; i < tasknum; i++) {
            if (strcmp(task_name, tasks[i].name) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            return 0; // 未找到任务，批处理序列无效
        }
    }
    return 1; // 所有任务名均有效
}


// // 批处理执行函数
// void exec_batch(void){
//     int channel = 0; // 使用通道 0
//     char batch_seq [MAX_BATCH_SEQ_LEN + 1]; 
//     // 读取sd卡中的批处理序列
//     unsigned start_block = batch_offset_img / SECTOR_SIZE;
//     unsigned num_block = NBYTES2SEC(MAX_BATCH_SEQ_LEN + 1);
//     #pragma GCC diagnostic push
//     #pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
//     sd_read(
//         (unsigned)batch_seq, 
//         num_block, 
//         start_block
//     );
//     #pragma GCC diagnostic pop

//     // 解析批处理序列
//     int task_num = 0; // 成功执行的任务数
//     char *p = batch_seq;
//     char *task_name = p; // 当前任务名的起始位置
//     int last = 0; // 是否执行到最后一个任务
//     while (1) {
//         if(*p == '\0') last = 1; // 完成批处理执行
//         if(*p == ' ' || last){
//             *p = '\0'; // 将空格替换为字符串结束符
//             if(! strlen(task_name)) break; // 防止最后是空格
//             bios_putstr(COLOR_GREEN"Executing: ");
//             bios_putstr(task_name);
//             bios_putstr("\n\r"COLOR_RESET);
//             uint64_t entry_point = load_task_img(task_name);
//             if (entry_point != 0) {
//                 // 首个 App 启动时，开启写使能，重置写指针，并修改写操作的jump table
//                 if (task_num == 0) {
//                     pipe_op(channel, PIPE_WRITE, PIPE_OPEN, PIPE_SET);
//                 }else{
//                     // 其他 App 开启读写使能，重置读指针，并修改读写操作的jump table
//                     pipe_op(channel, PIPE_READ, PIPE_OPEN, PIPE_CUR);
//                     pipe_op(channel, PIPE_WRITE, PIPE_OPEN, PIPE_CUR); // 写指针不能重置，否则认为管道为空
//                 }
//                 // 执行 App (exec_app 内部会设置 a1 栈顶)
//                 exec_app(entry_point, BATCH); 
//                 // 执行后清理 (关闭读写通道，保留数据在管道中)
//                 // 对所有APP，写通道都要关闭
//                 pipe_op(channel, PIPE_WRITE, PIPE_CLOSE, PIPE_CUR);
//                 pipe_op(channel, PIPE_READ, PIPE_CLOSE, PIPE_CUR);
//                 task_num++;
//             } else {
//                 // App 加载失败，直接退出批处理
//                 bios_putstr(COLOR_RED);
//                 bios_putstr("[Error]: Failed to load batch app. Exiting batch mode.\n\r");
//                 bios_putstr(COLOR_RESET);
//                 return;
//             }
//             if(last) break; // 如果是最后一个任务，执行完后就退出
//             p++;
//             task_name = p; // 更新下一个任务名的起始位置
//             continue;
//         }
//         p++; // 任务名中间继续移动指针
//     }

//     bios_putstr(COLOR_GREEN);
//     bios_putstr("Batch processing finished.\n\r");
//     bios_putstr(COLOR_RESET);

//     clear_pipe(channel); // 清理管道数据
// }

// 要求用户输入批处理的程序序列，并在sd卡中写入批处理程序，有无效程序需要重输（输quit退出）
void write_batch(void){
    while (1) {
        bios_putstr(COLOR_MAGENTA);
        bios_putstr("Please input the batch sequence, or quit: ");
        bios_putstr(COLOR_RESET);
        char buffer[MAX_BATCH_SEQ_LEN + 1] = {0}; // 初始化buffer
        int len = process_input_echo(buffer, MAX_BATCH_SEQ_LEN);
        
        if (len >= MAX_BATCH_SEQ_LEN)
        {
            bios_putstr(COLOR_YELLOW);
            bios_putstr("[WARNING]: Batch sequence is too long, please try again.\n\r");
            bios_putstr(COLOR_RESET);
            continue;
        }
        buffer[len] = '\0';

        if (strcmp(buffer, quit_batch) == 0) {
            bios_putstr(COLOR_GREEN);
            bios_putstr("Exiting batch write mode.\n\r");
            bios_putstr(COLOR_RESET);
            return;
        }else if(is_valid_batch(buffer, len)){
            // 若为有效批处理序列，写入sd卡
            unsigned start_block = batch_offset_img / SECTOR_SIZE;
            unsigned num_block = NBYTES2SEC(len+1); // +1 for '\0'
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
            bios_sd_write(
                (unsigned)buffer,
                num_block, 
                start_block
            );
            #pragma GCC diagnostic pop
            bios_putstr(COLOR_GREEN);
            bios_putstr("Sucessfully write the batch sequence.\n\r");
            bios_putstr(COLOR_RESET);
            return;
        }else{
            // 非法序列，要求重输
            bios_putstr(COLOR_YELLOW);
            bios_putstr("[WARNING]: Invalid batch sequence, please try again.\n\r");
            bios_putstr(COLOR_RESET);
            continue;
        }
    }
}
// ----------------------------------------------
//=======================Tool Functions==========================
int process_input_echo(char *buffer, int max_name_len){
    int len = 0;
    while (1) {
        int ch;
        while ((ch = bios_getchar()) == -1) // 等待输入
            ;
        if(ch == '\r' || ch == '\n') {
            bios_putstr("\n\r");
            break; // 按下回车键，结束输入
        }
        else if (ch == '\b' || ch == 127) { // '\b' 是退格键, 127 是 Delete 键
            if (len > 0) {
                len--;
                bios_putstr("\b \b"); // 在屏幕上实现删除效果：光标退后一格 + 输出空格覆盖原字符 + 光标再退后一次
            }
        }
        else{
            bios_putchar(ch);
            if (len < max_name_len) buffer[len++] = (char)ch; // 放入缓冲区
        }
    }
    return len;
}


// // --------加载任务
// void load_task(void)
// {
//     while (1)
//     {
//         bios_putstr(COLOR_CYAN);
//         bios_putstr("Please input task name: ");
//         bios_putstr(COLOR_RESET);
//         char buffer[MAX_NAME_LEN + 1] = {0}; // 初始化buffer
//         int len = process_input_echo(buffer, MAX_NAME_LEN); // 处理输入

//         if (len >= MAX_NAME_LEN)
//         {
//             bios_putstr(COLOR_YELLOW);
//             bios_putstr("[WARNING]: Task name is too long, please try again.\n\r");
//             bios_putstr(COLOR_RESET);
//             continue;
//         }
//         buffer[len] = '\0';

//         if (strcmp(buffer, list) == 0) {
//             list_tasks();
//             continue;
//         }else if (strcmp(buffer, wb) == 0) {
//             write_batch();
//             continue;
//         }else if (strcmp(buffer, eb) == 0) {
//             exec_batch();
//             continue;
//         }

//         uint64_t entry_point = 0; // 重新初始化，以防直接按回车时会执行上一次输入的程序
//         if (len != 0) entry_point = load_task_img(buffer); // 空字符串不调用，因为无效任务信息用空字符串填充
//         exec_app(entry_point, SINGLE);
//     }
// }

// /************************************************************/