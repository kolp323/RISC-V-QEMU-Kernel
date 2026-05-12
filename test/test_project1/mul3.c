#include <kernel.h>
#define MAX_NUM_LEN 100
#define EOT 4

static int my_atoi(char* str){
    int sum = 0;
    char *p = str;
    while(*p != '\0'){
        sum = sum * 10 + (*p - '0');
        p++;
    }
    return sum;
}

static int my_itoa(int num, char* str){
    int len = 0;
    while(num){
        str[len++] = (num % 10) + '0';
        num /= 10;
    }
    // 反转字符串
    for(int i = 0; i < len / 2; ++i){
        char tmp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = tmp;
    }
    str[len] = '\0';
    return len;
}


int main(){
    // 获取输入字符串
    char buffer[MAX_NUM_LEN];
    char ch;
    int len = 0;
    while ((ch = bios_getchar()) != EOT && ch != '\n' && ch != '\r') {
        buffer[len++] = ch;
    }
    buffer[len] = '\0';
    // 转换为数字
    int num = my_atoi(buffer);
    num *= 3;

    // 转换回字符串并输出
    my_itoa(num, buffer);
    bios_putstr(buffer);
    bios_putstr("\n");
}