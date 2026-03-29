#include "shell.h"

void kernel_main(void* mboot_ptr) {
    // 1. Wipe the screen clean via the driver
    kclear();

    // 2. Hand control to the Shell
    // Note: We'll put the "Welcome" text inside launch_shell
    // to keep kernel.c as clean as possible.
    launch_shell();

    // Safety hang if shell ever exits
    while(1) {
        __asm__("hlt");
    }
}
