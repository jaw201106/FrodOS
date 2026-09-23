// keyboard.c
#include <stdint.h>

void kprint(const char* str);

// Minimalist look-up table map for common hardware scan codes
static const char scancode_table[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' '
};

// --- Shared Asynchronous Circular Input Ring Buffer ---
#define KEY_BUFFER_SIZE 256
static char key_buffer[KEY_BUFFER_SIZE];
static int key_head = 0;
static int key_tail = 0;

// Invoked by the kernel syscall handler to drain characters to user space safely
char keyboard_get_char(void) {
    if (key_head == key_tail) {
        return 0; // Buffer ring is empty
    }
    char c = key_buffer[key_tail];
    key_tail = (key_tail + 1) % KEY_BUFFER_SIZE;
    return c;
}

void keyboard_handler_main(void) {
    // 1. Read scan code directly from the PS/2 keyboard controller hardware port
    uint8_t scancode;
    __asm__ volatile("inb $0x60, %0" : "=a"(scancode));

    // 2. Filter out key release events (bit 7 set)
    if (!(scancode & 0x80)) {
        char c = scancode_table[scancode];
        if (c != 0) {
            // FIXED: Removed the direct kernel-space kprint echoes completely.
            // The user-space shell loop handles the explicit terminal output validation.
            int next_head = (key_head + 1) % KEY_BUFFER_SIZE;
            if (next_head != key_tail) {
                key_buffer[key_head] = c;
                key_head = next_head;
            }
        }
    }

    // 3. Signal End of Interrupt (EOI) to the Primary Programmable Interrupt Controller (PIC)
    __asm__ volatile("movb $0x20, %%al\noutb %%al, $0x20" ::: "ax");
}
