// kernel.c
#include "shell.h"
#include "fs.h"

void kernel_main(void* mboot_ptr) {
    // Initialize file system
    fs_init();
    
    // Welcome screen
    print_welcome();
    
    // Start shell
    launch_shell();
    
    while(1) { 
        __asm__("hlt"); 
    }
}