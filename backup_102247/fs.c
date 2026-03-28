// fs.c - Simple in-memory file system for FrodOS
#include "fs.h"
#include "shell.h"
#include <stddef.h>

filesystem_t fs;

void fs_init() {
    fs.file_count = 0;
    
    for(int i = 0; i < MAX_FILES; i++) {
        fs.files[i].used = 0;
        fs.files[i].size = 0;
    }
    
    fs_create("README");
    fs_write("README", "Welcome to FrodOS! Type 'help' for commands.");
    
    fs_create("hello.c");
    fs_write("hello.c", "#include <stdio.h>\nint main() {\n    printf(\"Hello from FrodOS!\\n\");\n    return 0;\n}");
    
    fs_create("kernel.c");
    fs_write("kernel.c", "// This is the kernel source\n// You can edit this file");
}

int fs_create(const char* name) {
    if(fs.file_count >= MAX_FILES) return -1;
    
    for(int i = 0; i < fs.file_count; i++) {
        if(fs.files[i].used && str_compare(fs.files[i].name, (char*)name) == 0) {
            return -2;
        }
    }
    
    int idx = fs.file_count++;
    for(int i = 0; i < MAX_FILENAME && name[i]; i++) {
        fs.files[idx].name[i] = name[i];
    }
    fs.files[idx].name[MAX_FILENAME - 1] = '\0';
    fs.files[idx].used = 1;
    fs.files[idx].size = 0;
    fs.files[idx].data[0] = '\0';
    
    return idx;
}

int fs_write(const char* name, const char* data) {
    for(int i = 0; i < fs.file_count; i++) {
        if(fs.files[i].used && str_compare(fs.files[i].name, (char*)name) == 0) {
            int len = 0;
            while(data[len] && len < MAX_FILE_SIZE - 1) {
                fs.files[i].data[len] = data[len];
                len++;
            }
            fs.files[i].data[len] = '\0';
            fs.files[i].size = len;
            return len;
        }
    }
    return -1;
}

int fs_read(const char* name, char* buffer, int max_size) {
    for(int i = 0; i < fs.file_count; i++) {
        if(fs.files[i].used && str_compare(fs.files[i].name, (char*)name) == 0) {
            int len = (fs.files[i].size < max_size) ? fs.files[i].size : max_size - 1;
            for(int j = 0; j < len; j++) {
                buffer[j] = fs.files[i].data[j];
            }
            buffer[len] = '\0';
            return len;
        }
    }
    return -1;
}

int fs_delete(const char* name) {
    for(int i = 0; i < fs.file_count; i++) {
        if(fs.files[i].used && str_compare(fs.files[i].name, (char*)name) == 0) {
            fs.files[i].used = 0;
            for(int j = i; j < fs.file_count - 1; j++) {
                fs.files[j] = fs.files[j + 1];
            }
            fs.file_count--;
            return 0;
        }
    }
    return -1;
}

void fs_list() {
    unsigned short* vga = (unsigned short*)0xB8000;
    int row = 20;
    
    if(fs.file_count == 0) {
        const char* msg = "No files found.";
        int pos = row * 80;
        for(int i = 0; msg[i]; i++) {
            vga[pos++] = (unsigned short)msg[i] | (0x0E << 8);
        }
        return;
    }
    
    for(int i = 0; i < fs.file_count; i++) {
        if(fs.files[i].used) {
            int pos = row * 80;
            for(int j = 0; fs.files[i].name[j]; j++) {
                vga[pos++] = (unsigned short)fs.files[i].name[j] | (0x0F << 8);
            }
            row++;
            if(row >= 25) break;
        }
    }
}
