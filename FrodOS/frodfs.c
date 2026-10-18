#include "frodfs.h"

static frodfs_entry_t storage[FS_MAX_FILES];

static int fs_strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

static void fs_strncpy(char* dest, const char* src, uint32_t n) {
    uint32_t i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

void frodfs_init(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        storage[i].used = 0;
        storage[i].size = 0;
        storage[i].name[0] = '\0';
    }
}

int frodfs_create(const char* name) {
    if (!name || name[0] == '\0') return -1;
    
    // Check if the file already exists
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (storage[i].used && fs_strcmp(storage[i].name, name) == 0) {
            return -2; // File already exists
        }
    }
    
    // Find an unallocated directory slot
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!storage[i].used) {
            fs_strncpy(storage[i].name, name, FS_MAX_NAME);
            storage[i].size = 0;
            storage[i].used = 1;
            return 0; // Success
        }
    }
    return -3; // Disk Full (Out of descriptors)
}

int frodfs_write(const char* name, const uint8_t* buffer, uint32_t size) {
    if (!name || !buffer || size > FS_MAX_SIZE) return -1;

    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (storage[i].used && fs_strcmp(storage[i].name, name) == 0) {
            // Write payload data to buffer
            for (uint32_t j = 0; j < size; j++) {
                storage[i].data[j] = buffer[j];
            }
            storage[i].size = size;
            return 0; // Success
        }
    }
    return -2; // File not found
}

int frodfs_read(const char* name, uint8_t* buffer, uint32_t max_size, uint32_t* out_size) {
    if (!name || !buffer || !out_size) return -1;

    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (storage[i].used && fs_strcmp(storage[i].name, name) == 0) {
            uint32_t read_bytes = (storage[i].size > max_size) ? max_size : storage[i].size;
            for (uint32_t j = 0; j < read_bytes; j++) {
                buffer[j] = storage[i].data[j];
            }
            *out_size = read_bytes;
            return 0; // Success
        }
    }
    return -2; // File not found
}

// Writes filenames out separated by newlines into a string buffer
int frodfs_list(char* out_names_buffer, uint32_t max_bytes) {
    if (!out_names_buffer || max_bytes == 0) return -1;
    
    uint32_t idx = 0;
    out_names_buffer[0] = '\0';

    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (storage[i].used) {
            // Check if name fits (plus newline character)
            uint32_t len = 0;
            while (storage[i].name[len] != '\0') len++;
            
            if (idx + len + 2 >= max_bytes) break; // Out of user space memory buffer bounds

            for (uint32_t j = 0; j < len; j++) {
                out_names_buffer[idx++] = storage[i].name[j];
            }
            out_names_buffer[idx++] = '\n';
        }
    }
    out_names_buffer[idx] = '\0';
    return 0;
}
