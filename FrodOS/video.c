#include "shell.h"

// 1. Hardware Helper
static inline void outb(unsigned short port, unsigned char val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

// VGA Memory Settings
unsigned short* vga_buffer = (unsigned short*)0xB8000;
int vga_cursor = 0;
unsigned char char_color = 0x07;

// 2. Hardware Cursor Movement
void update_cursor() {
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((vga_cursor >> 8) & 0xFF));
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(vga_cursor & 0xFF));
}

// 3. Color Control
void kset_color(unsigned char color) {
    char_color = color;
}

// 4. Scrolling Logic
void scroll() {
    for (int i = 0; i < 80 * 24; i++) {
        vga_buffer[i] = vga_buffer[i + 80];
    }
    for (int i = 80 * 24; i < 80 * 25; i++) {
        vga_buffer[i] = (unsigned short)' ' | (char_color << 8);
    }
    vga_cursor = 80 * 24;
    update_cursor();
}

// 5. The missing piece: put_char (Now Global for fat32.c)
void put_char(char c) {
    if (vga_cursor >= 80 * 25) {
        scroll();
    }

    if (c == '\n') {
        vga_cursor = (vga_cursor / 80 + 1) * 80;
    }
    else if (c == '\b') {
        if (vga_cursor > 0) {
            vga_cursor--;
            vga_buffer[vga_cursor] = (unsigned short)' ' | (char_color << 8);
        }
    }
    else {
        vga_buffer[vga_cursor++] = (unsigned short)c | (char_color << 8);
    }
    update_cursor();
}

// 6. The Main Print Function (Calls put_char)
void kprint(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        put_char(str[i]);
    }
}

// 7. Clear Screen
void kclear() {
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (unsigned short)' ' | (char_color << 8);
    }
    vga_cursor = 0;
    update_cursor();
}

// 8. Number Printer
void kprint_int(int n) {
    if (n == 0) { kprint("0"); return; }
    char buf[12];
    int i = 10;
    buf[11] = '\0';
    unsigned int num = (n < 0) ? -n : n;
    if (n < 0) put_char('-');
    while (num > 0) {
        buf[i--] = (num % 10) + '0';
        num /= 10;
    }
    kprint(&buf[i + 1]);
}
