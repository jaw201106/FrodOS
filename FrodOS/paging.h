#ifndef PAGING_H
#define PAGING_H

#include "types.h"

#define PAGE_SIZE          4096
#define ENTRIES_PER_TABLE  1024

// Page Table / Directory Entry flags
#define PAGE_PRESENT  0x1
#define PAGE_WRITE    0x2
#define PAGE_USER     0x4  // Crucial: Ring 3 apps require this bit set

// Clear alignment macros
#define PAGE_ALIGN_DOWN(addr) ((addr) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_UP(addr)   (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

void init_paging(void);
void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
void switch_page_directory(uint32_t* directory);

#endif
