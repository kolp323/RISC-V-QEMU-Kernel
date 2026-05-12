/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                  The shell acts as a task running in user mode.
 *       The main function is to make system calls through the user's output.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <stdio.h> // TODO: 添加了颜色转义序列
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <shell.h>

// #include <assert.h>

char cur_path[MAX_PATH_LEN];  

int main(void)
{
    memset(cur_path, 0, MAX_PATH_LEN);
    *cur_path = '/'; // 初始时在根目录
    sys_move_cursor(0, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");

    while (1)
    {
        printf("> root@UCAS_OS:");
        printf("%s$ ", cur_path);
        // printf("%d", current_running->cursor_x);
        // TODO [P3-task1]: call syscall to read UART port
        char buffer[BUF_SIZE + 1] = {0}; // 初始化buffer
        int len = process_input_echo(buffer, BUF_SIZE); // 处理输入
        if (len == 0) continue; // 直接回车
        if (len >= BUF_SIZE) {
            printf("[WARNING]: too long command\n\r");
            break; // 命令过长
        }
        
        // TODO [P3-task1]: parse input
        // note: backspace maybe 8('\b') or 127(delete)
        char command[MAX_NAME_LEN + 1] = {0};
        int argc = 0;
        char *argv[MAX_ARGS];
        parse_input(buffer, command, argv, &argc);

        // TODO [P3-task1]: ps, exec, kill, clear    
        if (strcmp(command, "exec") == 0) exec(argc, argv);
        else if (strcmp(command, "kill") == 0) kill(argc, argv);
        else if (strcmp(command, "ps") == 0) ps();
        else if (strcmp(command, "clear") == 0) clear();
        else if (strcmp(command, "list") == 0) list();
        else if (strcmp(command, "taskset") == 0) taskset(argc, argv);
        else if (strcmp(command, "free") == 0) free_mem(argc, argv);
        else if (strcmp(command, "mkfs") == 0) mkfs();
        else if (strcmp(command, "statfs") == 0) statfs();
        else if (strcmp(command, "cd") == 0) cd(argc, argv);
        else if (strcmp(command, "mkdir") == 0) mkdir(argc, argv);
        else if (strcmp(command, "rmdir") == 0) rmdir(argc, argv);
        else if (strcmp(command, "ls") == 0) ls(argc, argv);
        else if (strcmp(command, "touch") == 0) touch(argc, argv);
        else if (strcmp(command, "cat") == 0) cat(argc, argv);
        else if (strcmp(command, "ln") == 0) ln(argc, argv);
        else if (strcmp(command, "rm") == 0) rm(argc, argv);
        else if (strcmp(command, "") == 0) ;
        else {
            printf("[ERROR]: Unknown Command \"%s\"\n", command);
        }

        /************************************************************/
        // TODO [P6-task1]: mkfs, statfs, cd, mkdir, rmdir, ls

        // TODO [P6-task2]: touch, cat, ln, ls -l, rm
        /************************************************************/
    }

    return 0;
}
