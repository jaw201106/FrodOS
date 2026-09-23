#include "pmm.h"
#include "shell.h"

// 32768 bits / 32 bits per integer = 1024 integers
static uint32_t pmm_bitmap[BITMAP_SIZE / 32];
static uint32_t total_pages = 0;

// Internal Helpers
static void pmm_lock_page(uint32_t addr) {
    uint32_t page = addr / PAGE_SIZE;
    if (page < BITMAP_SIZE) {
        pmm_bitmap[page / 32] |= (1 << (page % 32));
    }
}

static void pmm_unlock_page(uint32_t addr) {
    uint32_t page = addr / PAGE_SIZE;
    if (page < BITMAP_SIZE) {
        pmm_bitmap[page / 32] &= ~(1 << (page % 32));
    }
}

// 1. Core Allocation: Handles continuous allocations safely
void* pmm_alloc_pages(uint32_t count) {
    if (count == 0) return (void*)0;

    for (uint32_t i = 0; i < (BITMAP_SIZE / 32); i++) {
        if (pmm_bitmap[i] == 0xFFFFFFFF) continue;

        for (int j = 0; j < 32; j++) {
            uint32_t start_page = i * 32 + j;
            uint32_t continuous_found = 0;

            // Check if 'count' number of consecutive pages are available
            for (uint32_t k = 0; k < count; k++) {
                uint32_t cur_page = start_page + k;
                if (cur_page >= BITMAP_SIZE) break;

                // If bit is 0, page is free
                if (!(pmm_bitmap[cur_page / 32] & (1 << (cur_page % 32)))) {
                    continuous_found++;
                } else {
                    break;
                }
            }

            if (continuous_found == count) {
                uint32_t addr = start_page * PAGE_SIZE;
                for (uint32_t k = 0; k < count; k++) {
                    pmm_lock_page(addr + (k * PAGE_SIZE));
                }
                return (void*)addr;
            }
        }
    }
    return (void*)0; // Out of memory
}

void* pmm_alloc_page(void) {
    return pmm_alloc_pages(1);
}

void pmm_free_pages(void* addr, uint32_t count) {
    uint32_t base_addr = (uint32_t)addr;
    for (uint32_t i = 0; i < count; i++) {
        pmm_unlock_page(base_addr + (i * PAGE_SIZE));
    }
}

void pmm_free_page(void* addr) {
    pmm_free_pages(addr, 1);
}

// 2. Hardware System Initialization Initialization
void pmm_init(uint32_t mem_size, uint32_t kernel_end_addr) {
    total_pages = mem_size / PAGE_SIZE;
    if (total_pages > BITMAP_SIZE) total_pages = BITMAP_SIZE;

    // Phase 1: Lock down absolutely everything to be defensively safe
    for (uint32_t i = 0; i < (BITMAP_SIZE / 32); i++) {
        pmm_bitmap[i] = 0xFFFFFFFF;
    }

    // Phase 2: Open up available RAM (Skipping the 0x0 to 1MB legacy zone entirely)
    uint32_t safe_start = 0x100000; // 1MB

    // Safety Catch: If the kernel ends above 1MB, push the free memory start line past it
    if (kernel_end_addr > safe_start) {
        // Page align the kernel end address upwards
        safe_start = (kernel_end_addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    }

    for (uint32_t addr = safe_start; addr < mem_size; addr += PAGE_SIZE) {
        if ((addr / PAGE_SIZE) < BITMAP_SIZE) {
            pmm_unlock_page(addr);
        }
    }
}

// Phase 3: Hardware Verification (Can be called from kernel_main if multiboot map is present)
void pmm_parse_mmap(void* mmap_addr, uint32_t mmap_length) {
    struct multiboot_mmap_entry* entry = (struct multiboot_mmap_entry*)mmap_addr;
    uint32_t end_addr = (uint32_t)mmap_addr + mmap_length;

    while ((uint32_t)entry < end_addr) {
        // Type 1 means usable RAM, if it's not type 1, lock it down!
        if (entry->type != 1) {
            uint32_t base = entry->base_addr_low;
            uint32_t length = entry->length_low;

            for (uint32_t addr = base; addr < (base + length); addr += PAGE_SIZE) {
                pmm_lock_page(addr);
            }
        }
        // Move pointer to the next structure block layout offset entry cleanly
        entry = (struct multiboot_mmap_entry*)((uint32_t)entry + entry->size + sizeof(entry->size));
    }
}
