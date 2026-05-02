#include "gdt.h"
#include "shell.h"
#include "pmm.h"
#include "speaker.h"

// We only need ONE kernel_main.
// Multiboot loaders (like GRUB) put the 'magic' value in EAX and 'mboot_ptr' in EBX.
void kernel_main(void* mboot_ptr, unsigned int magic) {
    // 1. Setup Essentials (GDT, IDT, Video)
    gdt_install(); // Memory Segments
    idt_install(); // Error Handling/Interrupts
    kclear();

    // 2. Initialize PMM
    // We check the magic 0x2BADB002 to ensure we are booted correctly
    if (magic == 0x2BADB002) {
        unsigned int* mboot = (unsigned int*)mboot_ptr;

        // mboot[1] is mem_lower, mboot[2] is mem_upper (in KB)
        unsigned int mem_upper_kb = mboot[2];

        // Initialize PMM with the detected RAM
        pmm_init(mem_upper_kb * 1024);

        kprint("PMM Initialized with ");
        kprint_int(mem_upper_kb / 1024);
        kprint(" MB of RAM.\n");
        beep();
    } else {
        kprint("Warning: I am a chud\n");
    }

    // 3. Launch Shell (Pass the mboot_ptr so shell commands can use it)
    launch_shell(mboot_ptr);
}
