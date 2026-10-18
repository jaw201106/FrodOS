#ifndef FRODFS_H
#define FRODFS_H

#include "types.h"

#define FS_MAX_FILES   32
#define FS_MAX_NAME    32
#define FS_MAX_SIZE    4096   // 4KB maximum per file for simple flat buffers

typedef struct {
    char name[FS_MAX_NAME];
    uint32_t size;
    uint8_t data[FS_MAX_SIZE];
    uint8_t used;             // 1 = Active, 0 = Deleted/Empty
} frodfs_entry_t;

// --- Core Filesystem Interface Drivers ---
void frodfs_init(void);
int  frodfs_create(const char* name);
int  frodfs_write(const char* name, const uint8_t* buffer, uint32_t size);
int  frodfs_read(const char* name, uint8_t* buffer, uint32_t max_size, uint32_t* out_size);
int  frodfs_list(char* out_names_buffer, uint32_t max_bytes);

#endif
