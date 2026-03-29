#include "shell.h"

unsigned short* vga_buffer = (unsigned short*)0xB8000;
int vga_cursor = 0;

void kclear() {
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (unsigned short)' ' | (0x07 << 8);
    }
    vga_cursor = 0;
}

void kprint(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            vga_cursor = (vga_cursor / 80 + 1) * 80;
        } else {
            vga_buffer[vga_cursor++] = (unsigned short)str[i] | (0x07 << 8);
        }
        // Basic wrap-around for now
        if (vga_cursor >= 80 * 25) vga_cursor = 0;
    }
}
