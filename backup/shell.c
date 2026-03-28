// shell.c - Complete version for FrodOS
#include "shell.h"
#include "fs.h"

// Hardware I/O
static inline unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

// Keyboard map
unsigned char keyboard_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

// String compare
int str_compare(char* s1, char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// Welcome screen
void print_welcome() {
    unsigned short* vga = (unsigned short*)0xB8000;
    for(int i = 0; i < 80 * 25; i++) vga[i] = (unsigned short)' ' | (0x07 << 8);

    const char* lines[] = {
        "***************************************",
        "*          Welcome to FrodOS          *",
        "*            Version 0.06             *",
        "*     Filesystem | Editor | Snake     *",
        "***************************************"
    };

    for(int l = 0; l < 5; l++) {
        int start_pos = (5 + l) * 80 + (80 - 39) / 2;
        for(int i = 0; lines[l][i] != '\0'; i++) {
            vga[start_pos + i] = (unsigned short)lines[l][i] | (0x0B << 8);
        }
    }
}

// Command: ls - List files
void cmd_ls(int* cursor) {
    unsigned short* vga = (unsigned short*)0xB8000;
    *cursor = (*cursor / 80 + 1) * 80;
    
    const char* title = "Files in FrodOS:";
    for(int i = 0; title[i]; i++) {
        vga[(*cursor)++] = (unsigned short)title[i] | (0x0E << 8);
    }
    *cursor = (*cursor / 80 + 1) * 80;
    
    fs_list();
    *cursor = (*cursor / 80 + fs.file_count + 2) * 80;
}

// Command: create - Create a file
void cmd_create(char* filename, int* cursor) {
    int result = fs_create(filename);
    unsigned short* vga = (unsigned short*)0xB8000;
    
    if(result >= 0) {
        const char* msg = "File created: ";
        for(int i = 0; msg[i]; i++) vga[(*cursor)++] = (unsigned short)msg[i] | (0x0A << 8);
        for(int i = 0; filename[i]; i++) vga[(*cursor)++] = (unsigned short)filename[i] | (0x0F << 8);
    } else if(result == -2) {
        const char* msg = "File already exists!";
        for(int i = 0; msg[i]; i++) vga[(*cursor)++] = (unsigned short)msg[i] | (0x0C << 8);
    } else {
        const char* msg = "Cannot create file (max files reached)";
        for(int i = 0; msg[i]; i++) vga[(*cursor)++] = (unsigned short)msg[i] | (0x0C << 8);
    }
}

// Command: edit - Edit a file
void cmd_edit(char* filename, int* cursor) {
    unsigned short* vga = (unsigned short*)0xB8000;
    char buffer[MAX_FILE_SIZE];
    
    int result = fs_read(filename, buffer, MAX_FILE_SIZE);
    if(result < 0) {
        const char* msg = "File not found!";
        for(int i = 0; msg[i]; i++) vga[(*cursor)++] = (unsigned short)msg[i] | (0x0C << 8);
        return;
    }
    
    // Display file content
    *cursor = (*cursor / 80 + 1) * 80;
    const char* header = "=== Editing: ";
    for(int i = 0; header[i]; i++) vga[(*cursor)++] = (unsigned short)header[i] | (0x0E << 8);
    for(int i = 0; filename[i]; i++) vga[(*cursor)++] = (unsigned short)filename[i] | (0x0F << 8);
    const char* header2 = " ===";
    for(int i = 0; header2[i]; i++) vga[(*cursor)++] = (unsigned short)header2[i] | (0x0E << 8);
    *cursor = (*cursor / 80 + 1) * 80;
    
    // Show current content
    for(int i = 0; buffer[i]; i++) {
        if(buffer[i] == '\n') {
            *cursor = (*cursor / 80 + 1) * 80;
        } else {
            vga[(*cursor)++] = (unsigned short)buffer[i] | (0x07 << 8);
        }
    }
    
    *cursor = (*cursor / 80 + 2) * 80;
    const char* prompt = "Enter new content (end with '###' on new line):";
    for(int i = 0; prompt[i]; i++) vga[(*cursor)++] = (unsigned short)prompt[i] | (0x0B << 8);
    *cursor = (*cursor / 80 + 1) * 80;
    
    // Simple line editor
    char new_content[MAX_FILE_SIZE] = {0};
    int content_idx = 0;
    char line[80];
    int line_idx = 0;
    
    while(1) {
        if(inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            if(!(scancode & 0x80)) {
                char c = keyboard_map[scancode];
                if(c == '\n') {
                    line[line_idx] = '\0';
                    if(str_compare(line, "###") == 0) {
                        break;
                    }
                    for(int i = 0; i < line_idx; i++) {
                        new_content[content_idx++] = line[i];
                    }
                    new_content[content_idx++] = '\n';
                    line_idx = 0;
                    *cursor = (*cursor / 80 + 1) * 80;
                } else if(c == '\b' && line_idx > 0) {
                    line_idx--;
                    (*cursor)--;
                    vga[*cursor] = (unsigned short)' ' | (0x07 << 8);
                } else if(c >= ' ' && line_idx < 79) {
                    line[line_idx++] = c;
                    vga[(*cursor)++] = (unsigned short)c | (0x0F << 8);
                }
            }
        }
    }
    
    fs_write(filename, new_content);
    *cursor = (*cursor / 80 + 1) * 80;
    const char* saved = "File saved!";
    for(int i = 0; saved[i]; i++) vga[(*cursor)++] = (unsigned short)saved[i] | (0x0A << 8);
}

// Command: type - View a file
void cmd_type(char* filename, int* cursor) {
    unsigned short* vga = (unsigned short*)0xB8000;
    char buffer[MAX_FILE_SIZE];
    
    int result = fs_read(filename, buffer, MAX_FILE_SIZE);
    if(result < 0) {
        const char* msg = "File not found!";
        for(int i = 0; msg[i]; i++) vga[(*cursor)++] = (unsigned short)msg[i] | (0x0C << 8);
        return;
    }
    
    *cursor = (*cursor / 80 + 1) * 80;
    for(int i = 0; buffer[i]; i++) {
        if(buffer[i] == '\n') {
            *cursor = (*cursor / 80 + 1) * 80;
        } else {
            vga[(*cursor)++] = (unsigned short)buffer[i] | (0x07 << 8);
        }
    }
}

// Command: help - Show help
void cmd_help(int* cursor) {
    unsigned short* vga = (unsigned short*)0xB8000;
    const char* commands[] = {
        "ver     - Show version",
        "clear   - Clear screen",
        "hi      - Say hello",
        "ls      - List files",
        "create  - Create a file (usage: create filename)",
        "edit    - Edit a file (usage: edit filename)",
        "type    - View a file (usage: type filename)",
        "delete  - Delete a file (usage: delete filename)",
        "snake   - Play Snake game",
        "help    - Show this help"
    };
    
    *cursor = (*cursor / 80 + 1) * 80;
    const char* title = "=== FrodOS Commands ===";
    for(int i = 0; title[i]; i++) vga[(*cursor)++] = (unsigned short)title[i] | (0x0E << 8);
    *cursor = (*cursor / 80 + 1) * 80;
    
    for(int i = 0; i < 10; i++) {
        for(int j = 0; commands[i][j]; j++) {
            vga[(*cursor)++] = (unsigned short)commands[i][j] | (0x0F << 8);
        }
        *cursor = (*cursor / 80 + 1) * 80;
    }
}

// Command processor
void process_command(char* buffer, int* cursor) {
    unsigned short* vga = (unsigned short*)0xB8000;
    *cursor = (*cursor / 80 + 1) * 80;
    
    // Parse command and argument
    char cmd[32] = {0};
    char arg[64] = {0};
    int i = 0;
    while(buffer[i] && buffer[i] != ' ' && i < 31) {
        cmd[i] = buffer[i];
        i++;
    }
    cmd[i] = '\0';
    
    if(buffer[i] == ' ') {
        i++;
        int j = 0;
        while(buffer[i] && j < 63) {
            arg[j++] = buffer[i++];
        }
        arg[j] = '\0';
    }
    
    if(str_compare(cmd, "ver") == 0) {
        const char* msg = "FrodOS V0.06 - With File System, Editor & Snake";
        for(int j=0; msg[j]; j++) vga[(*cursor)++] = (unsigned short)msg[j] | (0x0E << 8);
    }
    else if(str_compare(cmd, "clear") == 0) {
        for(int j=0; j < 80*25; j++) vga[j] = (unsigned short)' ' | (0x07 << 8);
        *cursor = 0;
    }
    else if(str_compare(cmd, "hi") == 0) {
        const char* msg = "Hello from the Frod!";
        for(int j=0; msg[j]; j++) vga[(*cursor)++] = (unsigned short)msg[j] | (0x0F << 8);
    }
    else if(str_compare(cmd, "ls") == 0) {
        cmd_ls(cursor);
    }
    else if(str_compare(cmd, "create") == 0 && arg[0] != '\0') {
        cmd_create(arg, cursor);
    }
    else if(str_compare(cmd, "edit") == 0 && arg[0] != '\0') {
        cmd_edit(arg, cursor);
    }
    else if(str_compare(cmd, "type") == 0 && arg[0] != '\0') {
        cmd_type(arg, cursor);
    }
    else if(str_compare(cmd, "delete") == 0 && arg[0] != '\0') {
        int result = fs_delete(arg);
        if(result == 0) {
            const char* msg = "File deleted: ";
            for(int j=0; msg[j]; j++) vga[(*cursor)++] = (unsigned short)msg[j] | (0x0A << 8);
            for(int j=0; arg[j]; j++) vga[(*cursor)++] = (unsigned short)arg[j] | (0x0F << 8);
        } else {
            const char* msg = "File not found!";
            for(int j=0; msg[j]; j++) vga[(*cursor)++] = (unsigned short)msg[j] | (0x0C << 8);
        }
    }
    else if(str_compare(cmd, "snake") == 0) {
        start_snake_game();
    }
    else if(str_compare(cmd, "help") == 0) {
        cmd_help(cursor);
    }
    else if(cmd[0] != '\0') {
        const char* err = "Unknown command. Type 'help' for commands.";
        for(int j=0; err[j]; j++) vga[(*cursor)++] = (unsigned short)err[j] | (0x0C << 8);
    }
    
    *cursor = (*cursor / 80 + 1) * 80;
    vga[(*cursor)++] = (unsigned short)'>' | (0x0A << 8);
    vga[(*cursor)++] = (unsigned short)' ' | (0x0A << 8);
}

// Launch shell
void launch_shell() {
    unsigned short* vga = (unsigned short*)0xB8000;
    int cursor = 10 * 80;
    char buffer[80];
    int buffer_idx = 0;
    
    // Print initial prompt
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