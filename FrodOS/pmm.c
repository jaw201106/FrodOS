#include <stdint.h>
#include "shell.h"

#define PAGE_SIZE 4096
#define BITMAP_SIZE 32768 // Manages up to 128MB
uint32_t pmm_bitmap[BITMAP_SIZE / 32];

// 1. Internal Helpers
void pmm_lock_page(uint32_t addr) {
    uint32_t page = addr / PAGE_SIZE;
    pmm_bitmap[page / 32] |= (1 << (page % 32));
}

void pmm_unlock_page(uint32_t addr) {
    uint32_t page = addr / PAGE_SIZE;
    pmm_bitmap[page / 32] &= ~(1 << (page % 32));
}

// 2. Public API
void* pmm_alloc_page() {
    for (uint32_t i = 0; i < (BITMAP_SIZE / 32); i++) {
        if (pmm_bitmap[i] != 0xFFFFFFFF) {
            for (int j = 0; j < 32; j++) {
                if (!(pmm_bitmap[i] & (1 << j))) {
                    uint32_t addr = (i * 32 + j) * PAGE_SIZE;
                    pmm_lock_page(addr);
                    return (void*)addr;
                }
            }
        }
    }
    return 0;
}

void pmm_free_page(void* addr) {
    pmm_unlock_page((uint32_t)addr);
}

void pmm_init(uint32_t mem_size) {
    // Lock everything initially
    for(uint32_t i = 0; i < (BITMAP_SIZE / 32); i++) {
        pmm_bitmap[i] = 0xFFFFFFFF;
    }

    // Unlock available RAM (starting at 1MB to avoid kernel/BIOS)
    for(uint32_t addr = 0x100000; addr < mem_size; addr += PAGE_SIZE) {
        if (addr < BITMAP_SIZE * PAGE_SIZE) {
            pmm_unlock_page(addr);
        }
    }
}
// remember that this will need updating later.
