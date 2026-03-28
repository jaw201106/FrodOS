#include "shell.h"

// 1. Hardware Bridge
static inline unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

// 2. Keyboard Map
unsigned char keyboard_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

// 3. String Compare
int str_compare(char* s1, char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// 4. Welcome Screen
void print_welcome() {
    unsigned short* vga = (unsigned short*)0xB8000;
    for(int i = 0; i < 80 * 25; i++) vga[i] = (unsigned short)' ' | (0x07 << 8);

    const char* lines[] = {
        "***************************************",
        "*          Welcome to FrodOS          *",
        "*            Version 0.03             *",
        "***************************************"
    };

    for(int l = 0; l < 4; l++) {
        int start_pos = (5 + l) * 80 + (80 - 39) / 2;
        for(int i = 0; lines[l][i] != '\0'; i++) {
            vga[start_pos + i] = (unsigned short)lines[l][i] | (0x0B << 8);
        }
    }
}

// 5. Command Processor
void process_command(char* buffer, int* cursor) {
    unsigned short* vga = (unsigned short*)0xB8000;
    *cursor = (*cursor / 80 + 1) * 80;

    if (str_compare(buffer, "ver") == 0) {
        const char* msg = "FrodOS V0.03 - Shell_Update";
        for(int i=0; msg[i]; i++) vga[(*cursor)++] = (unsigned short)msg[i] | (0x0E << 8);
    }
    else if (str_compare(buffer, "clear") == 0) {
        for(int i=0; i < 80*25; i++) vga[i] = (unsigned short)' ' | (0x07 << 8);
        *cursor = 0;
    }
    else if (str_compare(buffer, "hi") == 0) {
        const char* msg = "Hello from the Frod!";
        for(int i=0; msg[i]; i++) vga[(*cursor)++] = (unsigned short)msg[i] | (0x0F << 8);
    }
    else {
        const char* err = "Unknown command.";
        for(int i=0; err[i]; i++) vga[(*cursor)++] = (unsigned short)err[i] | (0x0C << 8);
    }

    *cursor = (*cursor / 80 + 1) * 80;
    vga[(*cursor)++] = (unsigned short)'>' | (0x0A << 8);
    vga[(*cursor)++] = (unsigned short)' ' | (0x0A << 8);
}

// 6. Launch Shell
void launch_shell() {
    unsigned short* vga = (unsigned short*)0xB8000;
    int cursor = 10 * 80;
    char buffer[80]; // Fixed: Array instead of single char
    int buffer_idx = 0;

    while(1) {
        if (inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            if (!(scancode & 0x80)) {
                char c = keyboard_map[scancode];
                if (c == '\n') {
                    buffer[buffer_idx] = '\0';
                    process_command(buffer, &cursor);
                    buffer_idx = 0;
                } else if (c == '\b' && buffer_idx > 0) {
                    buffer_idx--;
                    cursor--;
                    vga[cursor] = (unsigned short)' ' | (0x07 << 8);
                } else if (c >= ' ' && buffer_idx < 79) {
                    buffer[buffer_idx++] = c;
                    vga[cursor++] = (unsigned short)c | (0x0F << 8);
                }
            }
        }
    }
}
