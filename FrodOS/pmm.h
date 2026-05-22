#ifndef PMM_H
#define PMM_H

typedef __UINT32_TYPE__ uint32_t;

#define PAGE_SIZE 4096
#define BITMAP_SIZE 32768 // Manages up to 128MB of raw RAM

// Multiboot 1 memory map entry structure
__attribute__((packed)) struct multiboot_mmap_entry {
    uint32_t size;
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type; // 1 = Usable RAM, others = Reserved hardware
};

// Core API
void pmm_init(uint32_t mem_size, uint32_t kernel_end_addr);
void pmm_parse_mmap(void* mmap_addr, uint32_t mmap_length);
void* pmm_alloc_page(void);
void* pmm_alloc_pages(uint32_t count);
void pmm_free_page(void* addr);
void pmm_free_pages(void* addr, uint32_t count);

#endif
