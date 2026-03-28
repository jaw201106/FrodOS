// io.h - Hardware I/O functions
#ifndef IO_H
#define IO_H

extern unsigned char keyboard_map[128];

static inline unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

#endif
