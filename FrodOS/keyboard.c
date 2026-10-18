// keyboard.c
#include "types.h"

#define KB_DATA   0x60
#define KB_STATUS 0x64
#define PIC1_DATA 0x21

static inline uint8_t inb(uint16_t port) {
    uint8_t r;
    __asm__ volatile ("inb %1, %0" : "=a"(r) : "Nd"(port) : "memory");
    return r;
}
static inline void outb(uint16_t port, uint8_t v) {
    __asm__ volatile ("outb %0, %1" : : "a"(v), "Nd"(port) : "memory");
}

static const char sc_normal[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0, 'a','s','d','f','g','h','j','k','l',';','\'','`',
    0, '\\','z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' '
};

static const char sc_shift[128] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0, 'A','S','D','F','G','H','J','K','L',':','"','~',
    0, '|','Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' '
};

#define KEY_BUFFER_SIZE 256
static char key_buffer[KEY_BUFFER_SIZE];
static volatile int key_head = 0;
static volatile int key_tail = 0;

static volatile int shift_down = 0;
static volatile int caps_on    = 0;
static volatile int ext_prefix = 0;

void keyboard_init(void) {
    // Drop anything left in the controller's output buffer
    while (inb(KB_STATUS) & 0x01) inb(KB_DATA);
    // Unmask IRQ1 on the master PIC
    outb(PIC1_DATA, inb(PIC1_DATA) & (uint8_t)~0x02);
}

char keyboard_get_char(void) {
    if (key_head == key_tail) return 0;
    char c = key_buffer[key_tail];
    key_tail = (key_tail + 1) % KEY_BUFFER_SIZE;
    return c;
}

static void push_key(char c) {
    int next = (key_head + 1) % KEY_BUFFER_SIZE;
    if (next != key_tail) {
        key_buffer[key_head] = c;
        key_head = next;
    }
}

// EOI is sent by the irq1_handler stub in boot.s, not here.
void keyboard_handler_main(void) {
    uint8_t sc = inb(KB_DATA);

    if (sc == 0xE0) { ext_prefix = 1; return; }   // arrows, etc.: ignore for now
    if (ext_prefix) { ext_prefix = 0; return; }

    int released = sc & 0x80;
    uint8_t code = sc & 0x7F;

    if (code == 0x2A || code == 0x36) { shift_down = !released; return; }
    if (released) return;
    if (code == 0x3A) { caps_on = !caps_on; return; }

    char c = shift_down ? sc_shift[code] : sc_normal[code];
    // Caps Lock only affects letters
    if (caps_on) {
        if (c >= 'a' && c <= 'z') c -= 32;
        else if (c >= 'A' && c <= 'Z') c += 32;
    }
    if (c) push_key(c);
}
