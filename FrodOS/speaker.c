#include <stdint.h>

// Assembly wrappers to talk to hardware ports
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

// Simple busy-loop delay
void mini_delay(int loops) {
    for (volatile int i = 0; i < loops; i++);
}

void play_sound(uint32_t nFrequence) {
    uint32_t Div = 1193180 / nFrequence;
    outb(0x43, 0xB6);
    outb(0x42, (uint8_t)(Div));
    outb(0x42, (uint8_t)(Div >> 8));

    uint8_t tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(0x61, tmp | 3);
    }
}

void nosound() {
    uint8_t tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
}

void beep() {
    play_sound(1000);     // Start 1000Hz tone
    mini_delay(20000000);
    nosound();            // Stop the tone
}
