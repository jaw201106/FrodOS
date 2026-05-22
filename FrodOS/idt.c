#include "shell.h"
#include "io.h"        // Uses your standard centralized I/O assembly routines

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

// Declarations for external assembly functions mapped inside boot.s
extern void idt_load(void);
extern void exception13_handler(void); // General Protection Fault (Vector 13)
extern void irq0_handler(void);        // System Timer Clock Ticker (Vector 32)
extern void irq1_handler(void);        // System Keyboard Driver    (Vector 33)

void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel, unsigned char flags) {
    idt[num].base_low = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void idt_install(void) {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (unsigned int)&idt;

    // 1. Clear the IDT out thoroughly to catch any dead loops
    for(int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // 2. Standard PIC Cascading Initialization Remap Engine
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);

    // 3. Robust PIC Mask Adjustment: Enable BOTH Timer (Bit 0) and Keyboard (Bit 1)
    // Binary: 11111100 -> Hex: 0xFC (0 means unmasked/active, 1 means masked/ignored)
    outb(0x21, 0xFC);
    outb(0xA1, 0xFF); // Disable all legacy slave PIC channels securely

    // 4. Register CPU Core Fault Trap Exception Barriers
    idt_set_gate(13, (unsigned int)exception13_handler, 0x08, 0x8E);

    // 5. Register Asynchronous Hardware Intercept Vectors
    idt_set_gate(32, (unsigned int)irq0_handler, 0x08, 0x8E); // Vector 32 = PIT Timer (IRQ 0)
    idt_set_gate(33, (unsigned int)irq1_handler, 0x08, 0x8E); // Vector 33 = Keyboard  (IRQ 1)

    // 6. Safely load the IDT CPU register barrier boundaries
    idt_load();

    // REMOVED the early "sti" trigger!
    // This allows your main kernel loop to finish boot setup prior to firing clock ticks.
}
