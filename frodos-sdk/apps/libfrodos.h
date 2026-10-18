// apps/libfrodos.h
#ifndef LIBFRODOS_H
#define LIBFRODOS_H

typedef __SIZE_TYPE__ size_t;

int make_syscall(int num, unsigned int arg1);
void sys_print(const char* s);
void sys_yield(void);
char sys_read_char(void);
void sys_clear(void);
void sys_delay(int loops);

int user_strcmp(const char* a, const char* b);

#endif
