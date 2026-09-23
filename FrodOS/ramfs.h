#ifndef RAMFS_H
#define RAMFS_H

#include <stdint.h>
#include <stddef.h>

typedef struct {                              // Offset
    char filename[100];                       // 0
    char mode[8];                             // 100
    char uid[8];                              // 108
    char gid[8];                              // 116
    char size[12];                            // 124 (Octal text, null-terminated)
    char mtime[12];                           // 136
    char chksum[8];                           // 148
    char typeflag;                            // 156
    char linkname[100];                       // 157
    char magic[6];                            // 257
    char version[2];                          // 263
    char uname[32];                           // 265
    char gname[32];                           // 297
    char devmajor[8];                         // 329
    char devminor[8];                         // 337
    char prefix[155];                         // 345
    char padding[12];                         // 500
} __attribute__((packed)) tar_header_t;

void ramfs_init(uintptr_t initrd_start, uintptr_t initrd_end);

uint8_t* ramfs_read(const char* filename, size_t* out_size);

#endif
