// shell.c - FrodOS Shell
#include "shell.h"
#include "fs.h"
#include "io.h"
#include "snake.h"
#include "oregon.h"

unsigned char keyboard_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

int str_compare(char* s1, char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void print_welcome() {
    unsigned short* vga = (unsigned short*)0xB8000;
    for(int i = 0; i < 80 * 25; i++) vga[i] = (unsigned short)' ' | (0x07 << 8);
    
    const char* lines[] = {
        "***************************************",
        "*          Welcome to FrodOS          *",
        "*            Version 0.04             *",
        "*       Games | Editor | Shell        *",
        "*     Type 'help' for commands        *",
        "***************************************"
    };
    
    for(int l = 0; l < 6; l++) {
        int start_pos = (5 + l) * 80 + (80 - 39) / 2;
        for(int i = 0; lines[l][i]; i++) {
            vga[start_pos + i] = (unsigned short)lines[l][i] | (0x0B << 8);
        }
    }
}

void cmd_ver(int* cursor) {
    unsigned short* vga = (unsigned short*)0xB8000;
    const char* msg = "FrodOS V0.04 - Games & Editor";
    for(int i=0; msg[i]; i++) vga[(*cursor)++] = (unsigned short)msg[i] | (0x0E << 8);
}

void cmd_clear(int* cursor) {
    unsigned short* vga = (unsigned short*)0xB8000;
    for(int i=0; i < 80*25; i++) vga[i] = (unsigned short)' ' | (0x07 << 8);
    *cursor = 0;
}

void cmd_hi(int* cursor) {
    unsigned short* vga = (unsigned short*)0xB8000;
    const char* msg = "Hello from FrodOS!";
    for(int i=0; msg[i]; i++) vga[(*cursor)++] = (unsigned short)msg[i] | (0x0F << 8);
}

void cmd_ls(int* cursor) {
    unsigned short* vga = (unsigned short*)0xB8000;
    *cursor = (*cursor / 80 + 1) * 80;
    const char* title = "Files:";
    for(int i=0; title[i]; i++) vga[(*cursor)++] = (unsigned short)title[i] | (0x0E << 8);
    *cursor = (*cursor / 80 + 1) * 80;
    fs_list();
    *cursor = (*cursor / 80 + fs.file_count + 2) * 80;
}

void cmd_snake(int* cursor) {
    start_snake_game();
}

void cmd_oregon(int* cursor) {
    start_oregon_trail();
}

void cmd_poweroff(int* cursor) {
    unsigned short* vga = (unsigned short*)0xB8000;
    const char* msg = "System powering off...";
    for(int i = 0; msg[i]; i++) vga[(*cursor)++] = (unsigned short)msg[i] | (0x0F << 8);
    for(volatile int i = 0; i < 1000000; i++);
    __asm__ volatile ("cli");
    __asm__ volatile ("hlt");
}

void cmd_reboot(int* cursor) {
    unsigned short* vga = (unsigned short*)0xB8000;
    const char* msg = "System rebooting...";
    for(int i = 0; msg[i]; i++) vga[(*cursor)++] = (unsigned short)msg[i] | (0x0F << 8);
    for(volatile int i = 0; i < 1000000; i++);
    __asm__ volatile ("mov $0x64, %%dx\n\tmov $0xFE, %%al\n\tout %%al, %%dx" : : : "dx", "al");
    while(1);
}

void cmd_halt(int* cursor) {
    unsigned short* vga = (unsigned short*)0xB8000;
    const char* msg = "System halted.";
    for(int i = 0; msg[i]; i++) vga[(*cursor)++] = (unsigned short)msg[i] | (0x0F << 8);
    for(volatile int i = 0; i < 1000000; i++);
    __asm__ volatile ("cli");
    __asm__ volatile ("hlt");
    while(1);
}

void cmd_help(int* cursor) {
    unsigned short* vga = (unsigned short*)0xB8000;
    const char* commands[] = {
        "ver      - Show version",
        "clear    - Clear screen", 
        "hi       - Say hello",
        "ls       - List files",
        "snake    - Play Snake game",
        "oregon   - Play Oregon Trail",
        "poweroff - Power off",
        "reboot   - Reboot",
        "halt     - Halt CPU",
        "help     - Show help"
    };
    for(int c = 0; c < 10; c++) {
        for(int i=0; commands[c][i]; i++) {
            vga[(*cursor)++] = (unsigned short)commands[c][i] | (0x0F << 8);
        }
        *cursor = (*cursor / 80 + 1) * 80;
    }
}

void process_command(char* buffer, int* cursor) {
    unsigned short* vga = (unsigned short*)0xB8000;
    *cursor = (*cursor / 80 + 1) * 80;
    
    if(str_compare(buffer, "ver") == 0) cmd_ver(cursor);
    else if(str_compare(buffer, "clear") == 0) cmd_clear(cursor);
    else if(str_compare(buffer, "hi") == 0) cmd_hi(cursor);
    else if(str_compare(buffer, "ls") == 0) cmd_ls(cursor);
    else if(str_compare(buffer, "snake") == 0) cmd_snake(cursor);
    else if(str_compare(buffer, "oregon") == 0) cmd_oregon(cursor);
    else if(str_compare(buffer, "poweroff") == 0) cmd_poweroff(cursor);
    else if(str_compare(buffer, "reboot") == 0) cmd_reboot(cursor);
    else if(str_compare(buffer, "halt") == 0) cmd_halt(cursor);
    else if(str_compare(buffer, "help") == 0) cmd_help(cursor);
    else if(buffer[0] != '\0') {
        const char* err = "Unknown command. Type 'help'";
        for(int i=0; err[i]; i++) vga[(*cursor)++] = (unsigned short)err[i] | (0x0C << 8);
    }
    
    *cursor = (*cursor / 80 + 1) * 80;
    vga[(*cursor)++] = (unsigned short)'>' | (0x0A << 8);
    vga[(*cursor)++] = (unsigned short)' ' | (0x0A << 8);
}

void launch_shell() {
    unsigned short* vga = (unsigned short*)0xB8000;
    int cursor = 10 * 80;
    char buffer[80];
    int buffer_idx = 0;
    
    vga[cursor++] = (unsigned short)'>' | (0x0A << 8);
    vga[cursor++] = (unsigned short)' ' | (0x0A << 8);
    
    while(1) {
        if(inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            if(!(scancode & 0x80)) {
                char c = keyboard_map[scancode];
                if(c == '\n') {
                    buffer[buffer_idx] = '\0';
                    process_command(buffer, &cursor);
                    buffer_idx = 0;
                } else if(c == '\b' && buffer_idx > 0) {
                    buffer_idx--;
                    cursor--;
                    vga[cursor] = (unsigned short)' ' | (0x07 << 8);
                } else if(c >= ' ' && buffer_idx < 79) {
                    buffer[buffer_idx++] = c;
                    vga[cursor++] = (unsigned short)c | (0x0F << 8);
                }
            }
        }
    }
}
