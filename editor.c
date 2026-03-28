// editor.c - Simple text editor for FrodOS
#include "editor.h"
#include "io.h"
#include "fs.h"

static unsigned short* vga = (unsigned short*)0xB8000;
static int cursor_x = 0;
static int cursor_y = 0;
static char editor_buffer[80][24];
static char filename[64];

void editor_clear_screen() {
    for(int y = 0; y < 24; y++) {
        for(int x = 0; x < 80; x++) {
            vga[y * 80 + x] = (unsigned short)' ' | (0x07 << 8);
            editor_buffer[x][y] = '\0';
        }
    }
}

void editor_draw_status() {
    // Status bar
    for(int x = 0; x < 80; x++) {
        vga[24 * 80 + x] = (unsigned short)' ' | (0x70 << 8);
    }
    
    // Show filename
    int i;
    for(i = 0; filename[i] && i < 20; i++) {
        vga[24 * 80 + i] = (unsigned short)filename[i] | (0x70 << 8);
    }
    
    // Show help text
    const char* help = "Ctrl+S Save  Ctrl+X Exit";
    for(i = 0; help[i]; i++) {
        vga[24 * 80 + 60 + i] = (unsigned short)help[i] | (0x70 << 8);
    }
}

void editor_draw_buffer() {
    // Draw all lines
    for(int y = 0; y < 24; y++) {
        for(int x = 0; x < 80; x++) {
            char c = editor_buffer[x][y];
            if(c == '\0') c = ' ';
            vga[y * 80 + x] = (unsigned short)c | (0x07 << 8);
        }
    }
    editor_draw_status();
    
    // Draw cursor
    int pos = cursor_y * 80 + cursor_x;
    char current = editor_buffer[cursor_x][cursor_y];
    if(current == '\0') current = ' ';
    vga[pos] = (unsigned short)current | (0x0F << 8);
}

void editor_insert_char(char c) {
    if(cursor_x < 79) {
        // Shift right
        for(int x = 79; x > cursor_x; x--) {
            editor_buffer[x][cursor_y] = editor_buffer[x-1][cursor_y];
        }
        editor_buffer[cursor_x][cursor_y] = c;
        cursor_x++;
        editor_draw_buffer();
    }
}

void editor_backspace() {
    if(cursor_x > 0) {
        // Shift left
        for(int x = cursor_x-1; x < 79; x++) {
            editor_buffer[x][cursor_y] = editor_buffer[x+1][cursor_y];
        }
        editor_buffer[79][cursor_y] = '\0';
        cursor_x--;
        editor_draw_buffer();
    }
}

void editor_newline() {
    if(cursor_y < 23) {
        // Move lines down
        for(int y = 23; y > cursor_y; y--) {
            for(int x = 0; x < 80; x++) {
                editor_buffer[x][y] = editor_buffer[x][y-1];
            }
        }
        // Clear new line
        for(int x = 0; x < 80; x++) {
            editor_buffer[x][cursor_y+1] = '\0';
        }
        cursor_y++;
        cursor_x = 0;
        editor_draw_buffer();
    }
}

void editor_save_file() {
    char content[MAX_FILE_SIZE];
    int pos = 0;
    
    for(int y = 0; y < 24; y++) {
        for(int x = 0; x < 80; x++) {
            if(editor_buffer[x][y] != '\0') {
                content[pos++] = editor_buffer[x][y];
            }
        }
        if(y < 23) content[pos++] = '\n';
    }
    content[pos] = '\0';
    
    fs_write(filename, content);
    
    // Show save confirmation
    const char* saved = "File saved!";
    for(int i = 0; saved[i]; i++) {
        vga[23 * 80 + i] = (unsigned short)saved[i] | (0x0A << 8);
    }
}

void editor_load_file(char* fname) {
    char buffer[MAX_FILE_SIZE];
    int result = fs_read(fname, buffer, MAX_FILE_SIZE);
    
    if(result >= 0) {
        int x = 0, y = 0;
        for(int i = 0; buffer[i] && y < 24; i++) {
            if(buffer[i] == '\n') {
                y++;
                x = 0;
            } else if(x < 80) {
                editor_buffer[x][y] = buffer[i];
                x++;
            }
        }
    }
}

void start_nano(char* fname) {
    // Save screen
    unsigned short saved[80*25];
    for(int i = 0; i < 80*25; i++) {
        saved[i] = vga[i];
    }
    
    // Initialize
    int i;
    for(i = 0; fname[i] && i < 63; i++) {
        filename[i] = fname[i];
    }
    filename[i] = '\0';
    
    editor_clear_screen();
    editor_load_file(fname);
    cursor_x = 0;
    cursor_y = 0;
    editor_draw_buffer();
    
    int running = 1;
    while(running) {
        if(inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            if(!(scancode & 0x80)) {
                char c = keyboard_map[scancode];
                
                // Handle special keys (scancodes)
                if(scancode == 0x1F) { // S key for Ctrl+S
                    editor_save_file();
                }
                else if(scancode == 0x2D) { // X key for Ctrl+X
                    running = 0;
                }
                else if(scancode == 0x4B) { // Left arrow
                    if(cursor_x > 0) cursor_x--;
                    editor_draw_buffer();
                }
                else if(scancode == 0x48) { // Up arrow
                    if(cursor_y > 0) cursor_y--;
                    editor_draw_buffer();
                }
                else if(scancode == 0x50) { // Down arrow
                    if(cursor_y < 23) cursor_y++;
                    editor_draw_buffer();
                }
                else if(scancode == 0x4D) { // Right arrow
                    if(cursor_x < 79) cursor_x++;
                    editor_draw_buffer();
                }
                else if(c == '\b') {
                    editor_backspace();
                }
                else if(c == '\n') {
                    editor_newline();
                }
                else if(c >= ' ' && c <= '~') {
                    editor_insert_char(c);
                }
            }
        }
    }
    
    // Restore screen
    for(int i = 0; i < 80*25; i++) {
        vga[i] = saved[i];
    }
}
