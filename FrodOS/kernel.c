#include "gdt.h"       // Houses both your gdt_install and idt_install hooks
#include "shell.h"     // Houses launch_shell
#include "pmm.h"       // Houses pmm_init and pmm_parse_mmap
#include "speaker.h"   // Houses beep
#include "mbr.h"       // Houses mbr_parse
#include "fat32.h"     // Houses fat32_init

// Freestanding type protection layers
typedef __UINT32_TYPE__ uint32_t;

// Standard explicit external references for basic support routines
void kclear(void);
void kprint(const char* str);
void kprint_int(int num);

// Phase 1 External Support Declarations
void timer_init(void);

// External global boundary layout marker automatically fed from linker.ld
extern unsigned int _kernel_end;

// Corrected Multiboot 1 Specification structure mapping
__attribute__((packed)) struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;

    // Explicit padding to account for ELF section headers (Offsets 28-44)
    // This resolves structure shifting bugs under -O2 optimizations!
    uint32_t elf_sec_table[4];

    uint32_t mmap_length; // Correctly placed at byte offset 44
    uint32_t mmap_addr;   // Correctly placed at byte offset 48
};

// Defensive Hardware Trap: Executed by boot.s if a General Protection Fault happens
void gpf_handler_main(void) {
    asm volatile("cli"); // Disable interrupts immediately to protect storage disks
    kprint("\n!!! KERNEL PANIC: GENERAL PROTECTION FAULT (CPU TRAP 13) !!!\n");
    kprint("System processing halted defensively to prevent disk corruption.\n");
    for(;;) { asm volatile("hlt"); }
}

void kernel_main(void* mboot_ptr, unsigned int magic) {
    // 1. Core Hardware Infrastructure Setup
    gdt_install(); // Configures base segment segments
    idt_install(); // Hooks basic exception traps and gate descriptors
    kclear();      // Flushes VGA canvas window

    kprint("--- Booting Custom Kernel ---\n");

    // 2. Defensive Multiboot Verification
    if (magic != 0x2BADB002) {
        kprint("Kernel Panic: Bootloader is not Multiboot compliant!\n");
        asm volatile("cli");
        for(;;) { asm volatile("hlt"); }
    }

    if (mboot_ptr == 0) {
        kprint("Kernel Panic: Multiboot information pointer is null!\n");
        asm volatile("cli");
        for(;;) { asm volatile("hlt"); }
    }

    // Cast pointer map safely over memory context bounds
    struct multiboot_info* mbi = (struct multiboot_info*)mboot_ptr;

    // Capture the runtime tracking address boundary of your kernel code
    uint32_t kernel_end_address = (uint32_t)&_kernel_end;

    // Ensure basic memory layout limits are reported by the bootloader (Bit 0)
    if (mbi->flags & 0x00000001) {
        uint32_t total_ram_kb = mbi->mem_lower + mbi->mem_upper;

        // Initialize PMM securely, keeping free allocations well above active kernel code space
        pmm_init(mbi->mem_upper * 1024, kernel_end_address);

        kprint("PMM Base Map Initialized. RAM: ");
        kprint_int((int)(total_ram_kb / 1024));
        kprint(" MB tracked.\n");
        beep();
    } else {
        kprint("Warning: Bootloader did not report basic RAM limits. Fallback to 8MB.\n");
        pmm_init(8 * 1024 * 1024, kernel_end_address);
    }

    // Advanced Safety Phase: Parse BIOS Memory Map Hole Protections (Bit 6)
    if (mbi->flags & 0x00000040) {
        kprint("Locking hardware-reserved memory regions...\n");
        pmm_parse_mmap((void*)mbi->mmap_addr, mbi->mmap_length);
    } else {
        kprint("Warning: Multiboot memory map missing. Continuing on basic map flags.\n");
    }

    // 3. Automated Partition Detection and File System Mounting
    kprint("Probing ATA hard disk interfaces...\n");

    // Scan partition table sector blocks dynamically
    uint32_t fat32_partition_lba = mbr_parse();

    if (fat32_partition_lba != 0) {
        kprint("Mounting system root filesystem from LBA block: ");
        kprint_int((int)fat32_partition_lba);
        kprint("\n");

        // Initialize and bind internal FAT32 state context data paths
        fat32_init(fat32_partition_lba);
    } else {
        kprint("Warning: No bootable FAT32 partitions found on raw drive.\n");
    }

    // 4. Time Baseline Allocation & Global Interrupt Unmasking
    kprint("Initializing system clock ticker baseline (100Hz)...\n");
    timer_init(); // Arm the PIT hardware channel

    kprint("Unmasking CPU hardware interrupt channels globally...\n");
    asm volatile("sti"); // Sets the Interrupt Flag (IF), turning hardware interrupts on!

    // 5. Interface Drop Loop Processing Control Chain
    kprint("\nGoing to shell console...\n");
    kprint("\n");
    launch_shell(mboot_ptr);
}
