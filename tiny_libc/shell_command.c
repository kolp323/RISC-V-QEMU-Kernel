#include <shell.h>
#include <string.h>
#include <stdio.h> 
#include <unistd.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>

void exec(int argc, char **argv){
    if(argc < 1){
        printf("[ERROR]: No task name is given.\n");
    }
    int wait_sign = 1; // shell是否等待新加载的进程执行完毕
    if (argv[argc - 1][0] == '&') {
        argc--;
        wait_sign = 0;
    }
    pid_t pid = sys_exec(argv[0], argc, argv);
    
    // TODO: modify judge logic
    if (pid == 0)
        printf("[ERROR]: Executing task [%s] failed.\n", argv[0]);
    else
        printf("[INFO]: Task [%s] is running succesfully with pid=%d.\n", argv[0], pid);
    if (wait_sign) sys_waitpid(pid);
}

void kill(int argc, char **argv){
    int pid = my_atoi(argv[0]);
    sys_kill(pid);
}

void ps(){
    sys_ps();
}

void clear(){
    sys_clear();
    sys_move_cursor(0, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
}

void list(){
    sys_show_task();
}

void taskset(int argc, char **argv){
    int invalid_format = 0;
    int mask = 0;
    // taskset -p <mask> <pid>
    if (strcmp(argv[0], "-p") == 0) {
        if (argc < 3) {
            invalid_format = 1;
        }else if(strncmp(argv[1], "0x", 2) != 0){
            printf("[taskset] invalid mask format, must start with \"0x\".\n");
        }else {
            mask = my_atoi(argv[1] + 2); // 跳过0x
            int pid = (pid_t)my_atoi(argv[2]);
            sys_taskset(mask, NULL, pid, argc, argv); 
            return;
        }
    }else { // taskset <mask> <task_name>, 需要调用exec
        if (argc < 2) {
            invalid_format = 1;
        }else if(strncmp(argv[0], "0x", 2) != 0){
            printf("[taskset] invalid mask format, must start with \"0x\".\n");
        }else{
            mask = my_atoi(argv[0] + 2); // 跳过0x
            int exec_argc = argc - 1;
            char **exec_argv = argv + 1;
            sys_taskset(mask, argv[1], 0, exec_argc, exec_argv); 
            return;
        }
    }


    if(invalid_format){
        printf("[taskset] invalid command format, please use\n");
        printf("taskset <mask> <task_name>\n");
        printf("taskset -p <mask> <pid>\n");
    }

}

void free_mem(int argc, char **argv){
    // 解析参数，检查是否有 "-h"
    int human_readable = 0;
    if (argc > 0 && strcmp(argv[0], "-h") == 0) {
        human_readable = 1;
    }

    long free_mem = sys_free_mem();

    if (human_readable) {
        if (free_mem >= 1024 * 1024 * 1024) {
             printf("Free memory: %ld GB\n", free_mem / (1024 * 1024 * 1024));
        } else if (free_mem >= 1024 * 1024) {
             printf("Free memory: %ld MB\n", free_mem / (1024 * 1024));
        } else if (free_mem >= 1024) {
             printf("Free memory: %ld KB\n", free_mem / 1024);
        } else {
             printf("Free memory: %ld B\n", free_mem);
        }
    } else {
        printf("Free memory: %ld B\n", free_mem);
    }
}

void mkfs(){
    sys_mkfs();
}
void statfs(){
    sys_statfs();
}
void cd(int argc, char **argv){
    if (argc < 1){
        printf("cd: missing operand");
        return;
    }
    if (argc > 1){
        printf("cd: too many arguments");
        return;
    }
    // int sign = sys_cd(argv[0]);
    // printf("cd: sign %d\n", sign);
    if (sys_cd(argv[0]) == 0) merge_path(cur_path, argv[0]);
}
void mkdir(int argc, char **argv){
    int p_flag = 0;
    char *path = NULL;
    if (argc < 1){
        printf("mkdir: missing operand");
        return;
    }
    if (argc > 2){
        printf("mkdir: too many arguments");
        return;
    }

    if(argc == 1){
        path = argv[0];
    }else{ // argc == 2
        if (strcmp(argv[0], "-p") == 0) { p_flag = 1; path = argv[1]; }
        else if (strcmp(argv[1], "-p") == 0) { p_flag = 1; path = argv[0]; }
        else { printf("mkdir: invalid command argument: %s %s\n", argv[0], argv[1]); return; }
    }

    if (p_flag) {
        char buf[MAX_PATH_LEN];
        strncpy(buf, path, MAX_PATH_LEN - 1);
        // 逐级扫描路径
        for (int i = 0; buf[i] != '\0'; i++) {
            if (buf[i] == '/') {
                if (i == 0) continue; // 跳过开头的根目录 '/'
                buf[i] = '\0';
                sys_mkdir(buf); // 尝试创建中间目录（忽略已存在的报错）
                buf[i] = '/';
            }
        }
        sys_mkdir(path); // 创建最后一级
    } else {
        sys_mkdir(path);
    }
}
void rmdir(int argc, char **argv){
    int p_flag = 0;
    char *path = NULL;
    if (argc < 1){
        printf("rmdir: missing operand");
        return;
    }
    if (argc > 2){
        printf("rmdir: too many arguments");
        return;
    }

    // TODO: 实现 -p 解析
    if (argc == 1) {
        path = argv[0];
    } else {
        if (strcmp(argv[0], "-p") == 0) { p_flag = 1; path = argv[1]; }
        else if (strcmp(argv[1], "-p") == 0) { p_flag = 1; path = argv[0]; }
        else { printf("mkdir: invalid command argument: %s %s\n", argv[0], argv[1]); return; }
    }

    if (p_flag) {
        char buf[MAX_PATH_LEN];
        strncpy(buf, path, MAX_PATH_LEN - 1);
        // 只要当前层级删除成功就尝试上一层
        while (sys_rmdir(buf) == 0) {
            char *last_slash = strrchr(buf, '/');
            if (last_slash == NULL || last_slash == buf) break; // 到达顶层
            *last_slash = '\0'; // 截断路径，指向上级目录
        }
    } else {
        sys_rmdir(path);
    }
}
void ls(int argc, char **argv){
    char *path = cur_path;
    int option = 0;
    // TODO: 实现 -l 解析
    if (argc > 2){
        printf("touch: too many arguments");
        return;
    }
    if (argc == 0) {
        sys_ls(path, option);
        return;
    }

    if (argc == 1) {
        // ls -l 或 ls /path
        if (strcmp(argv[0], "-l") == 0) option = 1;
        else path = argv[0];
    } else if (argc == 2) {
        // ls -l /path 或 ls /path -l
        option = 1;
        if (strcmp(argv[0], "-l") == 0) path = argv[1];
        else if (strcmp(argv[1], "-l") == 0) path = argv[0];
        else {printf("ls: invalid command argument: %s %s\n", argv[0], argv[1]); return;}
    }
    sys_ls(path, option);
}

void touch(int argc, char **argv){
    if (argc < 1){
        printf("touch: missing file operand");
        return;
    }
    if (argc > 1){
        printf("touch: too many arguments");
        return;
    }
    int fd = sys_open(argv[0], O_RDWR);
    sys_close(fd);
}

void cat(int argc, char **argv) {
    if (argc < 1){
        printf("cat: missing file operand");
        return;
    }
    if (argc > 1){
        printf("cat: too many arguments");
        return;
    }
    int fd = sys_open(argv[0], O_RDONLY);
    if (fd < 0) {
        printf("cat: %s: No such file or directory\n", argv[0]);
        return;
    }

    char buf[256];
    int read_len;
    while ((read_len = sys_read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[read_len] = '\0';
        printf("%s", buf);
    }
    printf("\n");
    sys_close(fd);
}

void ln(int argc, char **argv) {
    if (argc < 2) {
        printf("ln: missing file operand\n");
        return;
    }
    sys_ln(argv[0], argv[1]);
}

void rm(int argc, char **argv) {
    if (argc < 1) {
        printf("rm: missing operand\n");
        return;
    }
    sys_rm(argv[0]);
}