#include "gdt.h"

// Expanded GDT bounds to 6 frames to hold the new TSS entry safely
struct gdt_entry gdt[6];
struct gdt_ptr gp;
struct tss_entry tss; // Global instance of our task state descriptor block

extern void gdt_flush();

// Native assembly line execution helper to arm the CPU's Task Register
static inline void load_task_register(unsigned short sel) {
    __asm__ volatile("ltr %0" : : "r"(sel));
}

// Low-overhead memory clearing routine fallback
static void gdt_memset(void* dest, int val, int count) {
    char* temp = (char*)dest;
    while(count--) *temp++ = val;
}

void gdt_set_gate(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}

void gdt_install() {
    gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gp.base = (unsigned int)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment (Selector 0x08)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment (Selector 0x10)
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code (Selector 0x18 | 3 = 0x1B)
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data (Selector 0x20 | 3 = 0x23)

    // --- Initialize Task State Segment ---
    gdt_memset(&tss, 0, sizeof(struct tss_entry));

    // Set up standard kernel execution values.
    // When an interrupt fires in Ring 3, the CPU jumps to this stack location!
    tss.ss0  = 0x10;        // Points to Kernel Data Segment (Index 2)
    tss.esp0 = 0x90000;     // Safe baseline physical location for the Ring 0 stack pointer

    // Setup the entry inside GDT descriptor space (Index 5)
    // 0x89 = Present, Ring 0 Executable, 32-bit Available TSS Type Flag
    // ... Inside gdt_install(), right after setting tss.esp0 ...
    unsigned int tss_base = (unsigned int)&tss;
    unsigned int tss_limit = sizeof(struct tss_entry) - 1;
    gdt_set_gate(5, tss_base, tss_limit, 0x89, 0x00);


    // Apply segment registers updates globally
    gdt_flush();

    // Load Task Register with Selector index 5 -> 5 * 8 = 40 (0x28)
    load_task_register(0x28);
}
