#ifndef FXE_H
#define FXE_H

#include <stdint.h>

#define FXE_MAGIC_0  'F'
#define FXE_MAGIC_1  'X'
#define FXE_MAGIC_2  'E'
#define FXE_MAGIC_3  0x01  // Version 1 tag

typedef struct {
    char     magic[4];      // FIXED: Must be an explicit 4-byte character array
    uint32_t entry_point;   // Virtual execution base (0x6000000)
    uint32_t text_offset;
    uint32_t text_size;
    uint32_t data_offset;
    uint32_t data_size;
    uint32_t bss_size;
    uint32_t stack_size;
} __attribute__((packed)) fxe_header_t;

#endif
