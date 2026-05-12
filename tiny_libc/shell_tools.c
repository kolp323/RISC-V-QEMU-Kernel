#include <shell.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

int my_atoi(char* str){
    int sum = 0;
    char *p = str;
    while(*p != '\0'){
        sum = sum * 10 + (*p - '0');
        p++;
    }
    return sum;
}

int process_input_echo(char *buffer, int max_name_len){
    int len = 0;
    while (1) {
        int ch;
        while ((ch = sys_getchar()) == -1) // 等待输入
            ;
        if(ch == '\r' || ch == '\n') {
            printf("\n\r");
            break; // 按下回车键，结束输入
        }else if (ch == '\b' || ch == '\177')
        {	
            if(len > 0){ 
                len--;
                printf("%c", ch); // 保证不删除提示符
            }
        }else{
            printf("%c", ch);
            if (len < max_name_len) buffer[len++] = (char)ch; // 放入缓冲区
        }
    }
    return len;
}

void parse_input(char *buffer, char *command, char **argv, int *argc_ptr){
    char *p = buffer; // 当前指针
    int argc = 0;
    int cmd_len = 0;
    int max_argc = MAX_ARGS; 

    // 跳过前导空格
    while (*p != '\0' && isspace(*p)) p++;

    // 空命令
    if (*p == '\0') {
        *argc_ptr = 0;
    }

    // 复制命令名（第一个空格前）到 command 数组
    while (*p != '\0' && !isspace(*p)) {
        if (cmd_len <= MAX_NAME_LEN) {
            command[cmd_len++] = *p;
        }
        p++;
    }
    command[cmd_len] = '\0'; 

    if (*p != '\0') {
        *p++ = '\0'; 
    }
    
    // 提取后续参数 
    while (*p != '\0' && argc < max_argc) {
        // 跳过空格
        while (*p != '\0' && isspace(*p)) {
            p++;
        }
        if (*p == '\0') break; // 没有更多参数
        // 否则为下一个参数的起始位置
        argv[argc++] = p; 
        // 找到当前参数的结束位置，空格或 '\0'
        while (*p != '\0' && !isspace(*p)) {
            p++;
        }
        // 记录为一个字符串
        if (*p != '\0') {
            *p++ = '\0'; 
        }
    }

    *argc_ptr = argc;
}

// 合并路径后更新cur_path
void merge_path(char* cur_path, char* tar){
    if (tar == NULL || strlen(tar) == 0) return;

    // 绝对路径：tar 以 '/' 开头，直接覆盖
    if (tar[0] == '/') {
        strncpy(cur_path, tar, MAX_PATH_LEN - 1);
        cur_path[MAX_PATH_LEN - 1] = '\0';
        return;
    }

    // 相对路径
    char temp_tar[MAX_PATH_LEN];
    strncpy(temp_tar, tar, MAX_PATH_LEN - 1);
    
    char *next_slash;
    char *token = temp_tar;
    while ((next_slash = strchr(token, '/')) != NULL) {
        *next_slash = '\0';
        merge_token(cur_path, token);
        token = next_slash + 1;
    }

    // 最后一个 token 末尾没有斜杠的情况
    if (*token != '\0') {
        merge_token(cur_path, token);
    }

    // 确保路径不以 '/' 结尾（除非是根目录）
    int final_len = strlen(cur_path);
    if (final_len > 1 && cur_path[final_len - 1] == '/') {
        cur_path[final_len - 1] = '\0';
    }

}

void merge_token(char *cur_path, char *token) {
    if (strcmp(token, "..") == 0) {
        // 非根目录时 删除末尾目录
        if (strcmp(cur_path, "/") != 0) { 
            char *last_slash = strrchr(cur_path, '/');
            if (last_slash == cur_path) cur_path[1] = '\0'; // 只剩根目录的情况
            else if (last_slash != NULL) *last_slash = '\0';
        }
    } else if (strcmp(token, ".") != 0 && strlen(token) > 0) {
        int len = strlen(cur_path);
        if (cur_path[len - 1] != '/') strcat(cur_path, "/");
        strcat(cur_path, token);
    }
}