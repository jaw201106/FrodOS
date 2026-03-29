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
    int i = 0;
    while (s1[i] != '\0' && s1[i] == s2[i]) {
        i++;
    }
    return (unsigned char)s1[i] - (unsigned char)s2[i];
}

// 4. Command Processor
void process_command(char* buffer) {
    kprint("\n");
    if (str_compare(buffer, "hi") == 0) {
        kprint("Frods are better than realists");
    } else if (str_compare(buffer, "ver") == 0) {
        kprint("FrodOS V0.04 - Bare Metal");
    } else if (str_compare(buffer, "help") == 0) {
        kprint("Commands: hi, ver, help, clear, reboot, testvid, halt, memtest");
    } else if (str_compare(buffer, "clear") == 0) {
        kclear();
        kprint("> ");
        return;
    } else if (str_compare(buffer, "reboot") == 0) {
        __asm__ volatile("outb %%al, $0x64" : : "a"(0xFE));
    }
    // NEW: Video Test
    else if (str_compare(buffer, "testvid") == 0) {
        for(int i=0; i<25; i++) kprint("Video Scroll Test Line...\n");
    }
    // NEW: Halt CPU
    else if (str_compare(buffer, "halt") == 0) {
        kprint("System Halted.");
        __asm__ volatile("hlt");
    }
    // NEW: Basic Memory Test (Writes/Reads from 16MB mark)
    else if (str_compare(buffer, "memtest") == 0) {
        volatile unsigned int* mem_ptr = (volatile unsigned int*)0x1000000;
        *mem_ptr = 0xDEADBEEF;
        if (*mem_ptr == 0xDEADBEEF) {
            kprint("Memory Test Passed at 0x1000000!");
        } else {
            kprint("Memory Test Failed!");
        }
    }
    else {
        kprint("Unknown command: ");
        kprint(buffer);
    }
    kprint("\n> ");
}

// 5. Shell Entry
void launch_shell() {
    // Print splash on entry
    kprint("***************************************\n");
    kprint("*          Welcome to FrodOS          *\n");
    kprint("*            Version 0.04             *\n");
    kprint("***************************************\n");

    char buffer[80];
    int buffer_idx = 0;

    kprint("\n> ");

    while(1) {
        if (inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            if (!(scancode & 0x80)) {
                char c = keyboard_map[scancode];
                if (c == '\n') {
                    buffer[buffer_idx] = '\0';
                    process_command(buffer);
                    buffer_idx = 0;
                } else if (c == '\b' && buffer_idx > 0) {
                    buffer_idx--;
                    // Manual screen backspace logic
                    extern int vga_cursor;
                    extern unsigned short* vga_buffer;
                    vga_cursor--;
                    vga_buffer[vga_cursor] = (unsigned short)' ' | (0x07 << 8);
                } else if (c >= ' ' && buffer_idx < 79) {
                    buffer[buffer_idx++] = c;
                    char t[2] = {c, '\0'};
                    kprint(t);
                }
            }
        }
    }
}
