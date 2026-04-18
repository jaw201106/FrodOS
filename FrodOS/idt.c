#include "shell.h"

struct idt_entry {
    unsigned short base_low;
    unsigned short sel;
    unsigned char  always0;
    unsigned char  flags;
    unsigned short base_high;
} __attribute__((packed));

struct idt_ptr {
    unsigned short limit;
    unsigned int   base;
} __attribute__((packed));

// Global instances
struct idt_entry idt[256];
struct idt_ptr idtp;

// Hardware abstraction for the PIC
static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

// Declarations for assembly functions
extern void idt_load();
extern void irq1_handler();

void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel, unsigned char flags) {
    idt[num].base_low = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void idt_install() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (unsigned int)&idt;

    // 1. Clear the IDT
    for(int i=0; i<256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // 2. Remap the PIC
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);

    // 3. Mask interrupts (FD = Only Keyboard IRQ1 is enabled)
    // This prevents the Timer (IRQ0) from crashing the kernel immediately
    outb(0x21, 0xFD);
    outb(0xA1, 0xFF);

    // 4. Register Keyboard at index 33
    idt_set_gate(33, (unsigned int)irq1_handler, 0x08, 0x8E);

    // 5. Load and Enable
    idt_load();
    __asm__ volatile("sti");
}
