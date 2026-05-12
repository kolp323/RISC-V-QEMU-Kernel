#ifndef SHELL_H
#define SHELL_H

#define SHELL_BEGIN 20
#define BUF_SIZE 256
#define MAX_NAME_LEN 15 // 最大命令长度
#define MAX_ARG_LEN  15 // 最大参数长度
#define MAX_ARGS 8  // 最大参数数量
#define MAX_PATH_LEN 80

int my_atoi(char* str);
int process_input_echo(char *buffer, int max_name_len);
// 将输入拆分为 “命令 额外参数”的形式，并统计命令行参数个数
void parse_input(char *buffer, char *command, char **argv, int *argc_ptr);
void merge_path(char* cur_path, char* tar);
void merge_token(char *cur_path, char *token);

void exec(int argc, char **argv);
void kill(int argc, char **argv);
void ps();
void clear();
void list();
void taskset(int argc, char **argv);
void free_mem(int argc, char **argv);
void mkfs();
void statfs();
void cd(int argc, char **argv);
void mkdir(int argc, char **argv);
void rmdir(int argc, char **argv);
void ls(int argc, char **argv);
void touch(int argc, char **argv);
void cat(int argc, char **argv);
void ln(int argc, char **argv);
void rm(int argc, char **argv);

extern char cur_path[MAX_PATH_LEN];
#endif