#include "types.h"
#include "gdt.h"       // gdt_install
#include "pmm.h"       // pmm_init, pmm_parse_mmap
#include "speaker.h"   // beep, play_sound, nosound
#include "shell.h"     // kprint, kprint_int, kclear, idt_install, ...
#include "frodfs.h"
#include "paging.h"
#include "fxe.h"

typedef __SIZE_TYPE__ size_t;

/* ---------- constants ---------- */

#define MULTIBOOT_MAGIC      0x2BADB002u
#define MB_FLAG_MEM          (1u << 0)
#define MB_FLAG_MODS         (1u << 3)
#define MB_FLAG_MMAP         (1u << 6)

#define USER_BASE            0x200000u    // shell.bin is loaded here
#define USER_MAX_IMAGE       0x9000000u    // refuse images larger than 1 MB
#define USER_STR_MAX         256

#define TAR_BLOCK            512u

/* ---------- types ---------- */

typedef struct {
    char filename[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];          // octal text
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

_Static_assert(sizeof(tar_header_t) == 512, "tar header must be 512 bytes");

struct __attribute__((packed)) multiboot_mod {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t cmdline;
    uint32_t pad;
};

struct __attribute__((packed)) multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t elf_sec_table[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
};

// Layout produced by pushad
typedef struct {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
} __attribute__((packed)) syscall_regs_t;

/* ---------- external symbols & cleanly placed prototypes ---------- */

void timer_init(void);
void keyboard_init(void);
char keyboard_get_char(void);
extern void jump_to_user_mode(uint32_t entry_point);
extern unsigned int _kernel_end;

static void launch_userland(void);

/* ---------- state ---------- */

static uint32_t ramfs_start = 0;
static uint32_t ramfs_end   = 0;

/* ---------- helpers ---------- */

static void halt_forever(void) {
    __asm__ volatile ("cli");
    for (;;) __asm__ volatile ("hlt");
}

static void panic(const char* msg) {
    kprint("Kernel Panic: ");
    kprint(msg);
    kprint("\n");
    halt_forever();
}

static int ramfs_strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

static size_t octal_to_int(const char* s, int max_chars) {
    size_t r = 0;
    for (int i = 0; i < max_chars; i++) {
        if (s[i] == '\0' || s[i] == ' ') break;
        if (s[i] < '0' || s[i] > '7') continue;
        r = (r << 3) + (size_t)(s[i] - '0');
    }
    return r;
}

// Overlap-safe copy (the RAMFS module may sit near the user load address)
static void* kernel_memmove(void* dest, const void* src, size_t n) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    if (d == s || n == 0) return dest;
    if (d < s) { while (n--) *d++ = *s++; }
    else       { d += n; s += n; while (n--) *--d = *--s; }
    return dest;
}

/* ---------- RAMFS (TAR in a multiboot module) ---------- */

void* ramfs_read(const char* filename, size_t* out_size) {
    if (ramfs_start == 0 || ramfs_start >= ramfs_end) return 0;

    // Tolerate archives created with "./name" entries
    if (filename[0] == '.' && filename[1] == '/') filename += 2;

    uint32_t addr = ramfs_start;
    while (addr + TAR_BLOCK <= ramfs_end) {
        const tar_header_t* h = (const tar_header_t*)addr;
        if (h->filename[0] == '\0') break;            // end-of-archive block

        size_t size = octal_to_int(h->size, 12);
        uint32_t data = addr + TAR_BLOCK;

        // Reject entries whose data would run past the module
        if (size > ramfs_end - data) break;

        const char* name = h->filename;
        if (name[0] == '.' && name[1] == '/') name += 2;

        if ((h->typeflag == '0' || h->typeflag == '\0') &&
            ramfs_strcmp(name, filename) == 0) {
            if (out_size) *out_size = size;
            return (void*)data;
            }

            uint32_t advance = TAR_BLOCK + (uint32_t)(((size + 511) / 512) * 512);
        if (advance > ramfs_end - addr) break;
        addr += advance;
    }
    return 0;
}

/* ---------- user pointer helpers ---------- */

static int user_ptr_ok(uint32_t addr) {
    return addr >= USER_BASE;       // keeps NULL and kernel addresses out
}

/* ---------- syscalls ---------- */

void syscall_handler_main(syscall_regs_t* regs) {
    uint32_t num = regs->eax;
    uint32_t val = regs->ebx;

    // Existing system structures (clipping case 1-8 for space visibility)
    switch (num) {
        case 1: if (user_ptr_ok(val)) kprint((const char*)val); break;
        case 2: __asm__ volatile ("sti; hlt"); break;
        case 3: regs->eax = (uint32_t)(unsigned char)keyboard_get_char(); break;
        case 4: kclear(); break;
        case 7: if (val > 0) play_sound(val); break;
        case 8: nosound(); break;

        /* ====== BRAND NEW FRODFS SYSTEM CALL LAYERS ====== */

        case 10: // Create File -> ebx = pointer to filename string
            if (user_ptr_ok(val)) {
                regs->eax = frodfs_create((const char*)val);
            } else { regs->eax = -1; }
            break;

        case 11: // Write File -> ebx = pointer to data packet structure
            if (user_ptr_ok(val)) {
                // Pass structural arguments wrapped nicely via standard layout references
                uint32_t* params = (uint32_t*)val;
                const char* fname = (const char*)params[0];
                const uint8_t* src_buf = (const uint8_t*)params[1];
                uint32_t fsize = params[2];

                if (user_ptr_ok((uint32_t)fname) && user_ptr_ok((uint32_t)src_buf)) {
                    regs->eax = frodfs_write(fname, src_buf, fsize);
                } else { regs->eax = -1; }
            }
            break;

        case 12: // Read File -> ebx = pointer to parameter structure
            if (user_ptr_ok(val)) {
                uint32_t* params = (uint32_t*)val;
                const char* fname = (const char*)params[0];
                uint8_t* dest_buf = (uint8_t*)params[1];
                uint32_t max_len = params[2];
                uint32_t* actual_out = (uint32_t*)params[3];

                if (user_ptr_ok((uint32_t)fname) && user_ptr_ok((uint32_t)dest_buf) && user_ptr_ok((uint32_t)actual_out)) {
                    regs->eax = frodfs_read(fname, dest_buf, max_len, actual_out);
                } else { regs->eax = -1; }
            }
            break;

        case 13: // List Directory Storage Content -> ebx = pointer to target character buffer
            if (user_ptr_ok(val)) {
                uint32_t* params = (uint32_t*)val;
                char* target_str = (char*)params[0];
                uint32_t max_buf_bytes = params[1];

                if (user_ptr_ok((uint32_t)target_str)) {
                    regs->eax = frodfs_list(target_str, max_buf_bytes);
                } else { regs->eax = -1; }
            }
            break;

        default:
            kprint("\nWarning: invalid syscall requested by user mode.\n");
            break;

        case 20: // Exec Call -> ebx = char* filename string
            if (user_ptr_ok(val)) {
                const char* app_name = (const char*)val;

                // 1. Allocate a clean, zeroed stack frame buffer to hold the file contents
                uint8_t file_buf[4096];
                uint32_t file_size = 0;

                // Clear the buffer to prevent dirty memory reads
                for (int i = 0; i < 4096; i++) file_buf[i] = 0;

                // 2. Read the file out of FrodFS. Request up to 4096 bytes directly.
                int lookup = frodfs_read(app_name, file_buf, 4096, &file_size);

                // If the file cannot be found or is smaller than our custom header structure, fail gracefully
                if (lookup < 0 || file_size < sizeof(fxe_header_t)) {
                    regs->eax = -2; // File missing or corrupt
                    break;
                }

                // 3. Map a pointer to parse the structured FXE header layout definitions
                fxe_header_t* header = (fxe_header_t*)file_buf;

                // 4. Verify our custom magic identifier tag bytes safely using direct array index checks
                if (header->magic[0] != FXE_MAGIC_0 || header->magic[1] != FXE_MAGIC_1 ||
                    header->magic[2] != FXE_MAGIC_2 || header->magic[3] != FXE_MAGIC_3) {
                    kprint("Exec Error: Invalid FXE binary execution signature.\n");
                regs->eax = -4; // Invalid format error
                break;
                    }

                    // 5. Dynamic Memory Mapping via Header Metadata
                    uint32_t app_virtual_base = header->entry_point; // Mapped cleanly at 0x6000000
                    uint32_t total_memory_needed = header->text_size + header->data_size + header->bss_size;
                    uint32_t app_pages = (total_memory_needed + 4095) / 4096;

                    // Map Code, Data, and BSS pages dynamically based on what the program needs
                    for (uint32_t i = 0; i < app_pages; i++) {
                        uint32_t v_addr = app_virtual_base + (i * 4096);
                        uint32_t p_frame = (uint32_t)pmm_alloc_pages(1);
                        if (!p_frame) panic("OOM allocating space for custom FXE image segments.");
                        map_page(v_addr, p_frame, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
                    }

                    // 6. Map the Stack explicitly based on how much RAM the program requested
                    uint32_t stack_virtual_base = 0x7000000; // Isolated stack zone far away
                    uint32_t stack_pages = (header->stack_size + 4095) / 4096;
                    for (uint32_t i = 0; i < stack_pages; i++) {
                        uint32_t v_addr = stack_virtual_base + (i * 4096);
                        uint32_t p_frame = (uint32_t)pmm_alloc_pages(1);
                        if (!p_frame) panic("OOM allocating app stack pages.");
                        map_page(v_addr, p_frame, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
                    }

                    // 7. Copy only the raw machine code payload (skipping past the header bytes) into 0x6000000
                    uint32_t payload_size = file_size - sizeof(fxe_header_t);
                    kernel_memmove((void*)app_virtual_base, file_buf + sizeof(fxe_header_t), payload_size);

                    kprint("FrodFS: Structured FXE verified. Executing program block...\n");

                    // 8. Pass control over to the Ring 3 application entry point!
                    jump_to_user_mode(header->entry_point);

                    regs->eax = 0;
            } else { regs->eax = -1; }
            break;

        case 21: // Exit Call -> Terminate active execution space and drop back to shell prompt
            kprint("\nFrodFS: Application terminated cleanly. Returning to user shell...\n");

                // Re-invoke your baseline userland launcher to spin up a clean prompt instance
        launch_userland();
        break;
    }
}

/* ---------- fault handler ---------- */

static void kprint_hex32(uint32_t v) {
    kprint("0x");
    kprint_hex8((uint8_t)(v >> 24));
    kprint_hex8((uint8_t)(v >> 16));
    kprint_hex8((uint8_t)(v >> 8));
    kprint_hex8((uint8_t)v);
}

void gpf_handler_main(uint32_t err, uint32_t eip, uint32_t cs) {
    __asm__ volatile ("cli");
    kprint("\n!!! KERNEL PANIC: GENERAL PROTECTION FAULT (CPU TRAP 13) !!!\n");
    kprint("Error code: "); kprint_hex32(err);
    kprint("\nEIP: ");      kprint_hex32(eip);
    kprint("\nCS: ");       kprint_hex32(cs);
    kprint("\n");
    halt_forever();
}

/* ---------- boot steps ---------- */

static void find_ramfs(const struct multiboot_info* mbi) {
    if ((mbi->flags & MB_FLAG_MODS) && mbi->mods_count > 0) {
        const struct multiboot_mod* mods =
        (const struct multiboot_mod*)mbi->mods_addr;
        if (mods[0].mod_end > mods[0].mod_start) {
            ramfs_start = mods[0].mod_start;
            ramfs_end   = mods[0].mod_end;
            kprint("RAMFS module found.\n");
            return;
        }
    }
    kprint("Warning: no RAMFS/initrd module loaded by bootloader.\n");
}

static void init_memory(const struct multiboot_info* mbi) {
    uint32_t reserved_end = (uint32_t)&_kernel_end;
    if (ramfs_end > reserved_end) reserved_end = ramfs_end;

    if (mbi->flags & MB_FLAG_MEM) {
        uint32_t total_kb = mbi->mem_lower + mbi->mem_upper;
        pmm_init(mbi->mem_upper * 1024, reserved_end);
        kprint("PMM initialized. RAM: ");
        kprint_int((int)(total_kb / 1024));
        kprint(" MB tracked.\n");
        beep();
    } else {
        kprint("Warning: no RAM size from bootloader. Falling back to 8MB.\n");
        pmm_init(8 * 1024 * 1024, reserved_end);
    }

    if (mbi->flags & MB_FLAG_MMAP) {
        kprint("Locking hardware-reserved memory regions...\n");
        pmm_parse_mmap((void*)mbi->mmap_addr, mbi->mmap_length);
    } else {
        kprint("Warning: no multiboot memory map. Using basic map.\n");
    }
}

static void launch_userland(void) {
    size_t size = 0;
    void* image = ramfs_read("shell.bin", &size);

    if (!image) panic("RAMFS shell.bin missing from TAR stream!");

    kprint("Mapping virtual isolation memory windows for userland image...\n");

    // Calculate how many 4KB pages your shell binary needs
    uint32_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint32_t i = 0; i < num_pages; i++) {
        uint32_t virtual_page = USER_BASE + (i * PAGE_SIZE);

        // CHANGED: Use pmm_alloc_pages(1) to allocate 1 physical page frame
        uint32_t physical_frame = (uint32_t)pmm_alloc_pages(1);

        if (!physical_frame) {
            panic("OOM: Out of physical memory frames for shell allocation!");
        }

        // Map the page into the user-space page directory
        map_page(virtual_page, physical_frame, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    }

    kprint("Copying application instructions safely into mapped pages...\n");
    kernel_memmove((void*)USER_BASE, image, size);

    kprint("Passing control to Ring 3 via virtual page layout pointers...\n");
    jump_to_user_mode(USER_BASE);

    halt_forever();
}

/* ---------- entry point ---------- */

void kernel_main(void* mboot_ptr, unsigned int magic) {
    gdt_install();
    idt_install();

    kprint("--- Booting Custom Kernel ---\n");

    if (magic != MULTIBOOT_MAGIC) panic("Bootloader is not Multiboot compliant!");
    if (mboot_ptr == 0)           panic("Multiboot info pointer is null!");

    const struct multiboot_info* mbi = (const struct multiboot_info*)mboot_ptr;

    find_ramfs(mbi);
    init_memory(mbi);

    kprint("Initializing system timer (100Hz)...\n");
    timer_init();

    kprint("Initializing FrodFS scratch memory storage table...\n");
    frodfs_init();

    kprint("FrodFS Bootstrap: Dynamically importing all files from boot archive...\n");

    uint32_t current_tar_addr = ramfs_start;

    while (current_tar_addr + 512 <= ramfs_end) {
        const tar_header_t* header = (const tar_header_t*)current_tar_addr;

        if (header->filename[0] == '\0') {
            break;
        }

        size_t file_size = octal_to_int(header->size, 12);
        uint32_t file_data_ptr = current_tar_addr + 512;

        // 1. Locate the true filename by stripping out any folder paths (look for the last '/')
        const char* real_filename = header->filename;
        int p_idx = 0;
        while (header->filename[p_idx] != '\0') {
            if (header->filename[p_idx] == '/') {
                real_filename = &header->filename[p_idx + 1];
            }
            p_idx++;
        }

        // 2. Process the file if it isn't the primary user shell binary, an empty path, or size 0
        if (ramfs_strcmp(real_filename, "shell.bin") != 0 && real_filename[0] != '\0' && file_size > 0) {

            if (file_data_ptr + file_size <= ramfs_end) {

                // 3. Strip the file extensions (e.g., "snake.fxe" -> "snake")
                char short_name[32];
                int n_idx = 0;
                while (real_filename[n_idx] != '\0' && real_filename[n_idx] != '.' && n_idx < 31) {
                    short_name[n_idx] = real_filename[n_idx];
                    n_idx++;
                }
                short_name[n_idx] = '\0';

                // 4. Register and save the raw executable code directly into FrodFS index nodes
                frodfs_create(short_name);
                frodfs_write(short_name, (const uint8_t*)file_data_ptr, (uint32_t)file_size);

                kprint("  -> Mounted program node: ");
                kprint(short_name);
                kprint("\n");
            }
        }

        size_t aligned_size = ((file_size + 511) / 512) * 512;
        current_tar_addr += 512 + aligned_size;
    }
    kprint("FrodFS Bootstrap complete. Core disk storage populated!\n");
    /* ========================================================================= */

    kprint("Initializing keyboard controller...\n");
    keyboard_init();

    kprint("Enabling CPU interrupts...\n");
    __asm__ volatile ("sti");

    launch_userland();
}

