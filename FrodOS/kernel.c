// kernel.c
#include "shell.h"

void kernel_main(void* mboot_ptr) {
    // 1. Initial screen clear
    // 2. Call the welcome screen
    print_welcome();

    // 3. Start the interactive shell
    launch_shell();

    // Safety hang
    while(1) { __asm__("hlt"); }
}
