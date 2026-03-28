// Simple test kernel
void kernel_main(void* mboot_ptr) {
    unsigned short* vga = (unsigned short*)0xB8000;
    
    // Clear screen with blue background
    for(int i = 0; i < 80 * 25; i++) {
        vga[i] = (unsigned short)' ' | (0x1F << 8);
    }
    
    // Write message
    const char* msg = "FrodOS is booting...";
    int pos = 12 * 80 + (80 - 23) / 2;
    for(int i = 0; msg[i]; i++) {
        vga[pos + i] = (unsigned short)msg[i] | (0x0F << 8);
    }
    
    const char* msg2 = "Kernel loaded successfully!";
    pos = 13 * 80 + (80 - 29) / 2;
    for(int i = 0; msg2[i]; i++) {
        vga[pos + i] = (unsigned short)msg2[i] | (0x0A << 8);
    }
    
    while(1) {
        __asm__("hlt");
    }
}
