// apps/libfrodos.c
#include "libfrodos.h"

int make_syscall(int num, unsigned int arg1) {
    int ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "0"(num), "b"(arg1) : "memory");
    return ret;
}

void sys_print(const char* s) { make_syscall(1, (unsigned int)s); }
void sys_yield(void)          { make_syscall(2, 0); }
char sys_read_char(void)      { return (char)make_syscall(3, 0); }
void sys_clear(void)          { make_syscall(4, 0); }

void sys_delay(int loops) {
    for (volatile int i = 0; i < loops; i++) {}
}

int user_strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}
