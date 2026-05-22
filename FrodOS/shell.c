#include "shell.h"
#include "pmm.h"
#include "fat32.h"
#include "speaker.h"

static inline uint16_t inw(uint16_t port) {
    uint16_t result;
    asm volatile("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outb(unsigned short port, unsigned char val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

#define BUFFER_SIZE 1024

// --- State Variables ---
int shift_pressed = 0;
int caps_lock = 0;
int shell_buffer_idx = 0;
char shell_buffer[BUFFER_SIZE];
void* global_mboot_ptr = (void*)0;

void process_command(char* buffer, void* mboot_ptr);

// --- Input Helpers ---
char to_uppercase(char c) {
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

void handle_backspace() {
    if (shell_buffer_idx > 0) {
        shell_buffer_idx--;
        kprint("\b");
    }
}

void handle_newline() {
    shell_buffer[shell_buffer_idx] = '\0';
    process_command(shell_buffer, global_mboot_ptr);
    shell_buffer_idx = 0;
}

// --- Keyboard Maps ---
unsigned char keyboard_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

unsigned char keyboard_map_shifted[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
};

void keyboard_handler_main() {
    unsigned char scancode = inb(0x60);
    outb(0x20, 0x20);

    if (!(scancode & 0x80)) {
        switch(scancode) {
            case 0x0E: handle_backspace(); break;
            case 0x1C:
                if (shell_buffer_idx > 0) {
                    handle_newline();
                } else {
                    kprint("\n");
                }
                kprint("> ");
                break;
            case 0x2A:
            case 0x36: shift_pressed = 1;  break;
            case 0x3A: caps_lock = !caps_lock; break;
            default: {
                // Safety check: ensure scancode is within map bounds
                if (scancode < 128) {
                    char c = shift_pressed ? keyboard_map_shifted[scancode] : keyboard_map[scancode];

                    if (c != 0) {
                        if (caps_lock) {
                            if (c >= 'a' && c <= 'z') c -= 32;
                            else if (c >= 'A' && c <= 'Z') c += 32;
                        }
                        put_char(c);

                        if (shell_buffer_idx < BUFFER_SIZE - 1) {
                            shell_buffer[shell_buffer_idx++] = c;
                        }
                    }
                }
                break;
            }
        }
    } else {
        unsigned char released_code = scancode & 0x7F;
        if (released_code == 0x2A || released_code == 0x36) shift_pressed = 0;
    }
}
// 4. Command Processor
#include "shell.h"
#include "gdt.h"
#include "ata.h"

void process_command(char* buffer, void* mboot_ptr) {
    kprint("\n");
    if (str_compare(buffer, "hi") == 0) {
        kprint("Sounds and speakers gng.\n");
        kprint("peak ik ik \n");
    }
    else if (str_compare(buffer, "butwouldyoulose") == 0) {
        kprint("Nah, I'd Win.");
    }
    else if (str_compare(buffer, "ver") == 0) {
        kprint("FrodOS V0.08 - Sounds and Beeps!");
    }
    else if (str_compare(buffer, "mewo") == 0) {
        kset_color(0x8F);
        beep();
        kclear();
        kprint ("                  302      302    \n");
        kprint ("                 00 |     00 |    \n");
        kprint ("               00  |00000000 |    \n");
        kprint ("            000000000000000000    \n");
        kprint ("          10000000000000000000    \n");
        kprint ("    00000 100000000000000000000   \n");
        kprint ("   000000000000000   80000   00   \n");
        kprint ("             00000000000000000    \n");
        kprint ("               00000000000000     \n");
        kset_color(0x20);
        kprint("irlmarrrcywo_");
        kset_color(0x07);
    }
    else if (str_compare(buffer, "help") == 0) {
        kprint("Commands: hi, ver, help, clear, reboot, testvid, halt, memtest, meminfo, gdtinfo, color, alloc, freeall, readmbr, fd, lf, type, sound, uptime, timer");
    }
    else if (str_compare(buffer, "timer") == 0) {
        // Access the global ticking counter variable tracking from timer.c
        extern volatile unsigned int system_ticks;

        kprint("Current System Uptime Ticks: ");
        kprint_int((int)system_ticks);
        kprint("\n");
    }
    else if (str_compare(buffer, "uptime") == 0) {
        uint32_t get_uptime_ms(void); // Forward declaration of uptime helper

        uint32_t total_ms = get_uptime_ms();
        uint32_t seconds = total_ms / 1000;
        uint32_t milliseconds = total_ms % 1000;

        kprint("System Uptime: ");
        kprint_int((int)seconds);
        kprint(".");
        kprint_int((int)milliseconds);
        kprint(" seconds\n");
    }
    else if (str_compare(buffer, "lf") == 0) {
        fat32_ls();
    }
    else if (str_compare(buffer, "sound") == 0) {
        kprint("Request for speaker use...\n");
        kprint("Beep 1\n");
        beep();
        kprint("Beep 2\n");
        beep();
        kprint("Command finish\n");
    }
    else if (str_compare_partial(buffer, "fd") == 0) {
        fat32_cd(buffer + 3);
    }
    else if (str_compare(buffer, "readmbr") == 0) {
        uint16_t sector_data[256];
        ata_read_sector(0, sector_data);
        kprint("Reading LBA 0...\n");
        uint16_t magic = sector_data[255];

        if (magic == 0xAA55) {
            kprint("um... signature 0x55AA found ig.\n");
        } else {
            kprint("Error: Disk IS GONE BROCHACHO Got: 0x");
            beep();
            kprint_int(magic);
        }
    }
    else if (str_compare_partial(buffer, "type") == 0) {
        fat32_cat(buffer + 4);
    }
    else if (str_compare(buffer, "alloc") == 0) {
        void* addr = pmm_alloc_page();
        if (addr) {
            kprint("Allocated 4KB page at: 0x");
            kprint_int((unsigned int)addr);
        } else {
            kprint("Out of memory!");
        }
    }

    else if (str_compare(buffer, "freeall") == 0) {
        kprint("Resetting PMM... (Use with caution)");
    }
    // --- COLOR COMMANDS ---
    else if (str_compare(buffer, "color 0") == 0) { kset_color(0x07); kprint("Color: Light Gray"); }
    else if (str_compare(buffer, "color 1") == 0) { kset_color(0x09); kprint("Color: Bright Blue"); }
    else if (str_compare(buffer, "color 2") == 0) { kset_color(0x0A); kprint("Color: Light Green"); }
    else if (str_compare(buffer, "color 3") == 0) { kset_color(0x0B); kprint("Color: Light Cyan"); }
    else if (str_compare(buffer, "color 4") == 0) { kset_color(0x0C); kprint("Color: Light Red"); }
    else if (str_compare(buffer, "color 5") == 0) { kset_color(0x0D); kprint("Color: Light Magenta"); }
    else if (str_compare(buffer, "color 6") == 0) { kset_color(0x0E); kprint("Color: Yellow"); }
    else if (str_compare(buffer, "color 7") == 0) { kset_color(0x0F); kprint("Color: White"); }
    else if (str_compare(buffer, "color inv34t3d") == 0) { kset_color(0xF0); kprint("Color: Secret!"); }
    else if (str_compare(buffer, "color ketchup") == 0) { kset_color(0x14); kprint("Color: Secret!"); }
    else if (str_compare(buffer, "color") == 0) {
        kprint("Usage: color <0-7>");
    }
    else if (str_compare(buffer, "clear") == 0) {
        kclear();
        kprint("> ");
        return;
    }
    else if (str_compare(buffer, "reboot") == 0) {
        __asm__ volatile("outb %%al, $0x64" : : "a"(0xFE));
    }
    else if (str_compare(buffer, "testvid") == 0) {
        for(int i=0; i<25; i++) kprint("Video Scroll Test Line...\n");
    }
    else if (str_compare(buffer, "meminfo") == 0) {
        unsigned int* mboot = (unsigned int*)mboot_ptr;
        unsigned int mem_low = mboot[1];
        unsigned int mem_high = mboot[2];

        kprint("Lower Memory (KB): "); kprint_int(mem_low); kprint("\n");
        kprint("Upper Memory (KB): "); kprint_int(mem_high); kprint("\n");
        kprint("Total RAM (MB): ");    kprint_int((mem_low + mem_high) / 1024);
    }
    else if (str_compare(buffer, "halt") == 0) {
        kset_color(0x0C); // Bright Red
        kprint("SYSTEM HALTED. Power off manually.");
        __asm__ volatile("cli");
        while(1) { __asm__ volatile("hlt"); }
    }
    else if (str_compare(buffer, "gdtinfo") == 0) {
        extern struct gdt_ptr gp; // idk
        kprint("GDT Limit: "); kprint_int(gp.limit);
        kprint("\nGDT Base: 0x"); kprint("Loaded");
    }
    else if (str_compare(buffer, "memtest") == 0) {
        volatile unsigned int* mem_ptr = (volatile unsigned int*)0x1000000;
        *mem_ptr = 0xDEADBEEF;
        if (*mem_ptr == 0xDEADBEEF) {
            kprint("Memory Test Passed at 0x1000000!");
        } else {
            kprint("Memory Test Failed oh noo!");
        }
    }
    else {
        kprint("Unknown command: ");
        kprint(buffer);
    }
        kprint("\n");
}


// 5. Shell Entry
void launch_shell(void* mboot_ptr) {
    global_mboot_ptr = mboot_ptr;

    kprint("***************************************\n");
    kprint("*          Welcome to FrodOS          *\n");
    kprint("*            Version 0.09             *\n");
    kprint("***************************************\n");
    kprint("\n> ");

    while(1) {
        asm volatile("hlt");
    }
}

int str_compare_partial(char* s1, char* s2) {
    int i = 0;
    while (s2[i] != '\0') {
        if (s1[i] != s2[i]) return (unsigned char)s1[i] - (unsigned char)s2[i];
        i++;
    }
    return 0;
}
int str_compare(char* s1, char* s2) {
    int i = 0;
    while (s1[i] != '\0' && s1[i] == s2[i]) {
        i++;
    }
    return (unsigned char)s1[i] - (unsigned char)s2[i];
}

