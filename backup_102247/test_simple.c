// Simple test kernel
#include "io.h"

unsigned char keyboard_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

void kernel_main(void* mboot_ptr) {
    unsigned short* vga = (unsigned short*)0xB8000;
    
    // Clear screen
    for(int i = 0; i < 80*25; i++) {
        vga[i] = (unsigned short)' ' | (0x07 << 8);
    }
    
    // Welcome message
    const char* msg = "FrodOS Test Mode - Type letters to see them appear";
    int x = (80 - 48) / 2;
    for(int i = 0; msg[i]; i++) {
        vga[10 * 80 + x + i] = (unsigned short)msg[i] | (0x0B << 8);
    }
    
    const char* prompt = "> ";
    vga[12 * 80 + 0] = (unsigned short)'>' | (0x0A << 8);
    vga[12 * 80 + 1] = (unsigned short)' ' | (0x0A << 8);
    
    int cursor_x = 2;
    int cursor_y = 12;
    char buffer[80];
    int buffer_idx = 0;
    
    while(1) {
        if(inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            if(!(scancode & 0x80)) {
                char c = keyboard_map[scancode];
                
                if(c == '\n') {
                    buffer[buffer_idx] = '\0';
                    cursor_y++;
                    cursor_x = 0;
                    if(cursor_y > 23) cursor_y = 12;
                    
                    // Echo the command
                    for(int i = 0; buffer[i]; i++) {
                        vga[cursor_y * 80 + cursor_x++] = (unsigned short)buffer[i] | (0x0F << 8);
                    }
                    
                    // New prompt
                    cursor_y++;
                    cursor_x = 0;
                    vga[cursor_y * 80 + 0] = (unsigned short)'>' | (0x0A << 8);
                    vga[cursor_y * 80 + 1] = (unsigned short)' ' | (0x0A << 8);
                    cursor_x = 2;
                    buffer_idx = 0;
                } 
                else if(c == '\b' && buffer_idx > 0) {
                    buffer_idx--;
                    cursor_x--;
                    vga[cursor_y * 80 + cursor_x] = (unsigned short)' ' | (0x07 << 8);
                } 
                else if(c >= ' ' && c <= '~' && buffer_idx < 79) {
                    buffer[buffer_idx++] = c;
                    vga[cursor_y * 80 + cursor_x++] = (unsigned short)c | (0x0F << 8);
                }
            }
        }
    }
}
