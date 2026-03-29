#include "shell.h"

unsigned short* vga_buffer = (unsigned short*)0xB8000;
int vga_cursor = 0;
unsigned char char_color = 0x07; // Start with Light Gray

void kset_color(unsigned char color) {
    char_color = color;
}

void kclear() {
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (unsigned short)' ' | (char_color << 8);
    }
    vga_cursor = 0;
}

void kprint(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            vga_cursor = (vga_cursor / 80 + 1) * 80;
        } else {
            // This line now uses the global char_color!
            vga_buffer[vga_cursor++] = (unsigned short)str[i] | (char_color << 8);
        }

        // Wrap around if we hit the bottom (V0.06 will have real scrolling)
        if (vga_cursor >= 80 * 25) vga_cursor = 0;
    }
}
void kprint_int(int n) {
    if (n == 0) {
        kprint("0");
        return;
    }

    char buf[12]; // Buffer for the number string
    int i = 11;
    buf[i--] = '\0';

    // Handle negative numbers if needed
    unsigned int num = (n < 0) ? -n : n;
    if (n < 0) kprint("-");

    while (num > 0) {
        buf[i--] = (num % 10) + '0';
        num /= 10;
    }

    kprint(&buf[i + 1]);
}

