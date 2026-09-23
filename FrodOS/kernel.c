#include "gdt.h"       // Houses both your gdt_install and idt_install hooks
#include "pmm.h"       // Houses pmm_init and pmm_parse_mmap
#include "speaker.h"   // Houses beep
#include "mbr.h"       // Houses mbr_parse
#include "fat32.h"     // Houses fat32_init

// Brand-New Integrated RAMFS Structures & Modules
typedef __UINT32_TYPE__ uint32_t;
typedef __SIZE_TYPE__ size_t;

// Standard POSIX TAR header layout (512 bytes)
typedef struct {
    char filename[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];                            // Octal text, null-terminated
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
} __attribute__((packed)) tar_header_t;

// Multiboot 1 Module Reference definition
__attribute__((packed)) struct multiboot_mod {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t cmdline;
    uint32_t pad;
};

// Explicitly define the structural layout mapped onto the stack frame by pushad
typedef struct {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
} __attribute__((packed)) syscall_regs_t;

// Freestanding type protection layers
void gdt_install(void);
void idt_install(void);  // FIXED: Explicitly declared here to permanently resolve the implicit compiler warning
void kclear(void);
void kprint(const char* str);
void kprint_int(int num);
void timer_init(void);

// Explicit declaration to ensure the build object references sync cleanly with boot.s
void keyboard_handler_main(void);
char keyboard_get_char(void); // Declared readout connector hook to link cleanly

// Assembly interface bridging to the privilege drop frame handler
extern void jump_to_user_mode(uint32_t entry_point);

extern unsigned int _kernel_end;

// RAMFS Trackers
static uint32_t ramfs_start = 0;
static uint32_t ramfs_end = 0;

// Internal Helpers
static int ramfs_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static size_t octal_to_int(const char* str, int max_chars) {
    size_t result = 0;
    for (int i = 0; i < max_chars; i++) {
        if (str[i] == '\0' || str[i] == ' ') break;
        if (str[i] < '0' || str[i] > '7') continue;
        result = (result << 3) + (str[i] - '0');
    }
    return result;
}

// Low-overhead memory duplication engine
static void* kernel_memcpy(void* dest, const void* src, size_t count) {
    char* dst8 = (char*)dest;
    char* src8 = (char*)src;
    while (count--) { *dst8++ = *src8++; }
    return dest;
}

// Scans memory range for target module files
void* ramfs_read(const char* filename, size_t* out_size) {
    if (ramfs_start == 0 || ramfs_start >= ramfs_end) return 0;
    uint32_t current_address = ramfs_start;

    while (current_address < ramfs_end) {
        tar_header_t* header = (tar_header_t*)current_address;
        if (header->filename[0] == '\0') break;

        size_t file_size = octal_to_int(header->size, 12);

        if ((header->typeflag == '0' || header->typeflag == '\0') &&
            ramfs_strcmp(header->filename, filename) == 0) {
            if (out_size) *out_size = file_size;
            return (void*)(current_address + 512);
            }
            size_t aligned_size = ((file_size + 511) / 512) * 512;
        current_address += 512 + aligned_size;
    }
    return 0;
}

// FIXED: Receives a perfect stack pointer reference directly into the pushad context frame
void syscall_handler_main(syscall_regs_t* regs) {
    uint32_t num = regs->eax;
    uint32_t val = regs->ebx;

    switch (num) {
        case 1: // Service 1: Safe String Output rendering
            if (val != 0) {
                kprint((const char*)val);
            }
            break;
        case 2: // Service 2: Thread execution yielding
            asm volatile("sti; hlt");
            break;
        case 3: // Service 3: NEW! Safe user character buffer polling
            // Overwrite saved userland EAX image frame with retrieved character data payload
            regs->eax = (uint32_t)keyboard_get_char();
            break;
        default:
            kprint("\nWarning: Invalid Syscall requested by User Mode client.\n");
            break;
    }
}

// Corrected Multiboot 1 Specification structure mapping with Modules (Bit 3)
__attribute__((packed)) struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count; // Bit 3 Flag Check
    uint32_t mods_addr;  // Pointer map array to multiboot_mod items

    uint32_t elf_sec_table[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
};

void gpf_handler_main(void) {
    asm volatile("cli");
    kprint("\n!!! KERNEL PANIC: GENERAL PROTECTION FAULT (CPU TRAP 13) !!!\n");
    kprint("System processing halted defensively to prevent disk corruption.\n");
    for(;;) { asm volatile("hlt"); }
}

void kernel_main(void* mboot_ptr, unsigned int magic) {
    gdt_install();
    idt_install();
    kclear();

    kprint("--- Booting Custom Kernel ---\n");

    if (magic != 0x2BADB002) {
        kprint("Kernel Panic: Bootloader is not Multiboot compliant!\n");
        asm volatile("cli"); for(;;) { asm volatile("hlt"); }
    }
    if (mboot_ptr == 0) {
        kprint("Kernel Panic: Multiboot information pointer is null!\n");
        asm volatile("cli"); for(;;) { asm volatile("hlt"); }
    }

    struct multiboot_info* mbi = (struct multiboot_info*)mboot_ptr;
    uint32_t kernel_end_address = (uint32_t)&_kernel_end;

    if (mbi->flags & 0x00000001) {
        uint32_t total_ram_kb = mbi->mem_lower + mbi->mem_upper;
        pmm_init(mbi->mem_upper * 1024, kernel_end_address);
        kprint("PMM Base Map Initialized. RAM: ");
        kprint_int((int)(total_ram_kb / 1024));
        kprint(" MB tracked.\n");
        beep();
    } else {
        kprint("Warning: Bootloader did not report basic RAM limits. Fallback to 8MB.\n");
        pmm_init(8 * 1024 * 1024, kernel_end_address);
    }

    if (mbi->flags & 0x00000040) {
        kprint("Locking hardware-reserved memory regions...\n");
        pmm_parse_mmap((void*)mbi->mmap_addr, mbi->mmap_length);
    } else {
        kprint("Warning: Multiboot memory map missing. Continuing on basic map flags.\n");
    }

    // --- Fixed Multiboot Module Structure Referencing Array Logic ---
    if ((mbi->flags & 0x00000008) && mbi->mods_count > 0) {
        struct multiboot_mod* mods = (struct multiboot_mod*)mbi->mods_addr;
        ramfs_start = mods[0].mod_start;
        ramfs_end = mods[0].mod_end;
        kprint("RAMFS storage module found and mapped into physical bounds.\n");
    } else {
        kprint("Warning: No RAMFS/Initrd modules loaded by bootloader.\n");
    }

    kprint("Probing ATA hard disk interfaces...\n");
    uint32_t fat32_partition_lba = mbr_parse();

    if (fat32_partition_lba != 0) {
        kprint("Mounting system root filesystem from LBA block: ");
        kprint_int((int)fat32_partition_lba); kprint("\n");
        fat32_init(fat32_partition_lba);
    } else {
        kprint("Warning: No bootable FAT32 partitions found on raw drive.\n");
    }

    kprint("Initializing system clock ticker baseline (100Hz)...\n");
    timer_init();

    kprint("Unmasking CPU hardware interrupt channels globally...\n");
    asm volatile("sti");

    // --- RAMFS Content Verification and Execution Flow Hook ---
    size_t parser_size = 0;
    void* parser_binary = ramfs_read("shell.bin", &parser_size);

    if (parser_binary != 0) {
        kprint("Extracted user space command parser binary via RAMFS successfully.\n");
        kprint("Parser Binary Size: ");
        kprint_int((int)parser_size);
        kprint(" bytes.\n");

        // Define a safe target destination in memory to map the application binary (e.g., 2MB area)
        uint32_t user_space_address = 0x200000;

        kprint("Deploying executable mapping to User space segment...\n");
        kernel_memcpy((void*)user_space_address, parser_binary, parser_size);

        kprint("Passing control to Ring 3 environment context...\n");

        // Execute the forged CPU interrupt return mechanism to switch rings!
        jump_to_user_mode(user_space_address);

        // Safety fallback block loop
        for(;;) { asm volatile("hlt"); }
    } else {
        kprint("CRITICAL ERROR: RAMFS shell.bin parser missing from TAR stream! Halting.\n");
        asm volatile("cli");
        for(;;) { asm volatile("hlt"); }
    }
}
