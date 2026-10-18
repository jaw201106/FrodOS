#include "paging.h"
#include "pmm.h" // Assumes pmm_alloc_block() or equivalent returns a free 4KB physical page frame
#include "shell.h"

// Statically align our core kernel page directory to a 4KB boundary
__attribute__((aligned(4096))) static uint32_t kernel_page_directory[ENTRIES_PER_TABLE];

// Allocate space for initial page tables to identity-map the kernel lower region
__attribute__((aligned(4096))) static uint32_t low_kernel_tables[3][ENTRIES_PER_TABLE];

void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t dir_index = virtual_addr >> 22;
    uint32_t table_index = (virtual_addr >> 12) & 0x3FF;

    // Check if the page table for this directory index already exists
    if (!(kernel_page_directory[dir_index] & PAGE_PRESENT)) {
        // Allocate a blank page frame via your PMM for the new page table
        // For bootstrapping, we are using pre-allocated low memory pools
        kprint("Paging Error: Dynamic table mapping requested but allocation unhandled.\n");
        return;
    }

    // Extract the physical page table pointer address (masking away flag bits)
    uint32_t* table = (uint32_t*)(kernel_page_directory[dir_index] & ~0xFFF);
    table[table_index] = (physical_addr & ~0xFFF) | flags;
}

void init_paging(void) {
    // 1. Clear out the page directory
    for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
        kernel_page_directory[i] = 0 | PAGE_WRITE; // Supervisor, read/write, not present
    }

    // 2. Clear out our early low page tables
    for (int t = 0; t < 3; t++) {
        for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
            low_kernel_tables[t][i] = 0;
        }
    }

    // 3. Connect low tables to the directory
    // Maps the first 12MB of RAM (0x0 to 0xC00000) to keep kernel space intact
    for (uint32_t t = 0; t < 3; t++) {
        kernel_page_directory[t] = ((uint32_t)&low_kernel_tables[t]) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
        
        for (uint32_t i = 0; i < ENTRIES_PER_TABLE; i++) {
            uint32_t physical_address = (t * ENTRIES_PER_TABLE + i) * PAGE_SIZE;
            // Identity map: virtual address matches physical address exactly
            low_kernel_tables[t][i] = physical_address | PAGE_PRESENT | PAGE_WRITE;
        }
    }

    // 4. Load the page directory into the CPU's CR3 register
    switch_page_directory(kernel_page_directory);

    // 5. Flip the paging enablement bit in CR0 register via inline assembly
    __asm__ volatile (
        "mov %%cr0, %%eax\n"
        "or $0x80000000, %%eax\n"
        "mov %%eax, %%cr0\n"
        ::: "eax"
    );
    kprint("VMM Engine Activated: MMU Hardware 32-Bit Paging is now Online.\n");
}

void switch_page_directory(uint32_t* directory) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(directory));
}
