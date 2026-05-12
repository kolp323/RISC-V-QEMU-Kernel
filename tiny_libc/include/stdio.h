#ifndef INCLUDE_STDIO_H_
#define INCLUDE_STDIO_H_

#include <stdarg.h>

// 定义了常用的 ANSI 颜色转义序列，用于终端输出彩色文本
#define COLOR_RED      "\e[31m"
#define COLOR_GREEN    "\e[32m"
#define COLOR_YELLOW   "\e[33m"
#define COLOR_BLUE     "\e[34m"
#define COLOR_MAGENTA  "\e[35m"
#define COLOR_CYAN     "\e[36m"
#define COLOR_RESET    "\e[0m"


/* modes of sys_open */
#define O_RDONLY 1  /* read only open */
#define O_WRONLY 2  /* write only open */
#define O_RDWR   3  /* read/write open */

/* whence of sys_lseek */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list va);

#endif
