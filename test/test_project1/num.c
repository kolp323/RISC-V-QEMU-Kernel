#include <kernel.h>
#define MAX_NUM_LEN 100

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

int main(int argc, char** argv){
    int start_num = 5;
    char str[MAX_NUM_LEN];
    my_itoa(start_num, str);
    bios_putstr(str);
    bios_putstr("\n");
}