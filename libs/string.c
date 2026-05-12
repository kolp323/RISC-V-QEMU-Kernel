#include <os/string.h>

/**
 * @brief 复制内存内容
 * @param dest 目标内存地址
 * @param src  源内存地址
 * @param len  复制的字节数
 */
void memcpy(uint8_t *dest, const uint8_t *src, uint32_t len)
{
    for (; len != 0; len--) {
        *dest++ = *src++;
    }
}

/**
 * @brief 将内存设置为指定的值
 * @param dest 目标内存地址
 * @param val  设置的值
 * @param len  设置的字节数
 */
void memset(void *dest, uint8_t val, uint32_t len)
{
    uint8_t *dst = (uint8_t *)dest;

    for (; len != 0; len--) {
        *dst++ = val;
    }
}

/**
 * @brief 将内存清零
 * @param dest 目标内存地址
 * @param len  清零的字节数
 */
void bzero(void *dest, uint32_t len)
{
    memset(dest, 0, len);
}

/**
 * @brief 计算字符串长度
 * @param src 字符串指针
 * @return 字符串长度（不包括结尾的 '\0'）
 */
int strlen(const char *src)
{
    int i = 0;
    while (src[i] != '\0') {
        i++;
    }
    return i;
}

/**
 * @brief 比较两个字符串
 * @param str1 字符串1
 * @param str2 字符串2
 * @return 相等返回0，不等返回差值
 */
int strcmp(const char *str1, const char *str2)
{
    while (*str1 && *str2) {
        if (*str1 != *str2) {
            return (*str1) - (*str2);
        }
        ++str1;
        ++str2;
    }
    return (*str1) - (*str2);
}

/**
 * @brief 比较两个字符串的前n个字符
 * @param str1 字符串1
 * @param str2 字符串2
 * @param n    比较的字符数
 * @return 相等返回0，不等返回差值
 */
int strncmp(const char *str1, const char *str2, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i)
        if (str1[i] == '\0' || str1[i] != str2[i])
            return str1[i] - str2[i];
    return 0;
}

/**
 * @brief 拷贝字符串
 * @param dest 目标字符串
 * @param src  源字符串
 * @return 目标字符串指针
 */
char *strcpy(char *dest, const char *src)
{
    char *tmp = dest;

    while (*src) {
        *dest++ = *src++;
    }

    *dest = '\0';

    return tmp;
}

/**
 * @brief 拷贝字符串的前n个字符
 * @param dest 目标字符串
 * @param src  源字符串
 * @param n    拷贝的字符数
 * @return 目标字符串指针
 */
char *strncpy(char *dest, const char *src, int n)
{
    char *tmp = dest;

    while (*src && n-- > 0) {
        *dest++ = *src++;
    }

    while (n-- > 0) {
        *dest++ = '\0';
    }

    return tmp;
}

/**
 * @brief 字符串拼接
 * @param dest 目标字符串（拼接结果）
 * @param src  源字符串
 * @return 目标字符串指针
 */
char *strcat(char *dest, const char *src)
{
    char *tmp = dest;

    while (*dest != '\0') {
        dest++;
    }
    while (*src) {
        *dest++ = *src++;
    }

    *dest = '\0';

    return tmp;
}

char* strchr(char* str, char c){
    while (*str != c) {
        if (*str == '\0') return NULL;
        str++;
    }
    return str;
}

char* strrchr(char* str, char c) {
    char* last = NULL;
    do {
        if (*str == c) last = str;
    } while (*str++ != '\0');
    
    return last;
}

