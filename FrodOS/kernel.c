#include "gdt.h"
#include "shell.h"

void kernel_main(void* mboot_ptr) {
    gdt_install(); // Memory Segments
    idt_install(); // Error Handling
    kclear();

    // Pass the mboot_ptr to the shell so it can read the RAM map
    launch_shell(mboot_ptr);
}
