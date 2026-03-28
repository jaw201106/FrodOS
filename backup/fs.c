// fs.c - Simple in-memory file system for FrodOS
#include "fs.h"
#include "shell.h"
#include <stddef.h>

// Global file system instance
filesystem_t fs;

// Initialize the file system
void fs_init() {
    fs.file_count = 0;
    
    // Clear all file entries
    for(int i = 0; i < MAX_FILES; i++) {
        fs.files[i].used = 0;
        fs.files[i].size = 0;
    }
    
    // Create some initial files
    fs_create("README");
    fs_write("README", "Welcome to FrodOS! Type 'help' for commands.");
    
    fs_create("hello.c");
    fs_write("hello.c", "#include <stdio.h>\nint main() {\n    printf(\"Hello from FrodOS!\\n\");\n    return 0;\n}");
    
    fs_create("kernel.c");
    fs_write("kernel.c", "// This is a copy of kernel.c\n// You can edit this file");
}

// Create a new file
// Returns: index on success, -1 if max files reached, -2 if file exists
int fs_create(const char* name) {
    // Check if we have room for more files
    if(fs.file_count >= MAX_FILES) return -1;
    
    // Check if file already exists
    for(int i = 0; i < fs.file_count; i++) {
        if(fs.files[i].used && str_compare(fs.files[i].name, (char*)name) == 0) {
            return -2; // File exists
        }
    }
    
    // Create the new file
    int idx = fs.file_count++;
    
    // Copy the filename
    for(int i = 0; i < MAX_FILENAME && name[i]; i++) {
        fs.files[idx].name[i] = name[i];
    }
    fs.files[idx].name[MAX_FILENAME - 1] = '\0'; // Ensure null termination
    
    // Initialize file
    fs.files[idx].used = 1;
    fs.files[idx].size = 0;
    fs.files[idx].data[0] = '\0';
    
    return idx;
}

// Write data to a file
// Returns: number of bytes written, -1 if file not found
int fs_write(const char* name, const char* data) {
    for(int i = 0; i < fs.file_count; i++) {
        if(fs.files[i].used && str_compare(fs.files[i].name, (char*)name) == 0) {
            // Copy data into file
            int len = 0;
            while(data[len] && len < MAX_FILE_SIZE - 1) {
                fs.files[i].data[len] = data[len];
                len++;
            }
            fs.files[i].data[len] = '\0';
            fs.files[i].size = len;
            return len; // Return bytes written
        }
    }
    return -1; // File not found
}

// Read data from a file
// Returns: number of bytes read, -1 if file not found
int fs_read(const char* name, char* buffer, int max_size) {
    for(int i = 0; i < fs.file_count; i++) {
        if(fs.files[i].used && str_compare(fs.files[i].name, (char*)name) == 0) {
            // Calculate how many bytes to copy
            int len = (fs.files[i].size < max_size) ? fs.files[i].size : max_size - 1;
            
            // Copy file data to buffer
            for(int j = 0; j < len; j++) {
                buffer[j] = fs.files[i].data[j];
            }
            buffer[len] = '\0'; // Null terminate
            
            return len; // Return bytes read
        }
    }
    return -1; // File not found
}

// Delete a file
// Returns: 0 on success, -1 if file not found
int fs_delete(const char* name) {
    for(int i = 0; i < fs.file_count; i++) {
        if(fs.files[i].used && str_compare(fs.files[i].name, (char*)name) == 0) {
            // Mark file as unused
            fs.files[i].used = 0;
            
            // Shift remaining files to fill the gap
            for(int j = i; j < fs.file_count - 1; j++) {
                fs.files[j] = fs.files[j + 1];
            }
            fs.file_count--;
            return 0; // Success
        }
    }
    return -1; // File not found
}

// List all files in the file system
void fs_list() {
    unsigned short* vga = (unsigned short*)0xB8000;
    int row = 20;
    
    // If no files, show message
    if(fs.file_count == 0) {
        const char* msg = "No files found.";
        int pos = row * 80;
        for(int i = 0; msg[i]; i++) {
            vga[pos++] = (unsigned short)msg[i] | (0x0E << 8);
        }
        return;
    }
    
    // Display each file
    for(int i = 0; i < fs.file_count; i++) {
        if(fs.files[i].used) {
            int pos = row * 80;
            
            // Display filename
            for(int j = 0; fs.files[i].name[j]; j++) {
                vga[pos++] = (unsigned short)fs.files[i].name[j] | (0x0F << 8);
            }
            
            // Convert file size to string
            char size_str[16];
            int size = fs.files[i].size;
            int si = 0;
            
            if(size == 0) {
                size_str[si++] = '0';
            } else {
                // Count digits
                int temp = size;
                int digits = 0;
                while(temp > 0) {
                    temp /= 10;
                    digits++;
                }
                
                // Convert each digit
                temp = size;
                for(int d = digits - 1; d >= 0; d--) {
                    size_str[d] = '0' + (temp % 10);
                    temp /= 10;
                }
                si = digits;
            }
            size_str[si] = '\0';
            
            // Display file size in parentheses
            vga[pos++] = (unsigned short)' ' | (0x08 << 8);
            vga[pos++] = (unsigned short)'(' | (0x08 << 8);
            for(int j = 0; size_str[j]; j++) {
                vga[pos++] = (unsigned short)size_str[j] | (0x08 << 8);
            }
            vga[pos++] = (unsigned short)')' | (0x08 << 8);
            
            row++;
            if(row >= 25) break; // Don't go past screen bottom
        }
    }
}