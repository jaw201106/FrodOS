#ifndef GDT_H
#define GDT_H

// Standard x86 TSS structure definition
struct tss_entry {
    unsigned int prev_tss;
    unsigned int esp0;       // Kernel Stack Pointer (What the CPU looks for!)
    unsigned int ss0;        // Kernel Stack Segment (Index 2 -> 0x10)
    unsigned int esp1; unsigned int ss1; unsigned int esp2; unsigned int ss2;
    unsigned int cr3;  unsigned int eip; unsigned int eflags;
    unsigned int eax;  unsigned int ecx; unsigned int edx; unsigned int ebx;
    unsigned int esp;  unsigned int ebp; unsigned int esi; unsigned int edi;
    unsigned int es;   unsigned int cs;  unsigned int ss;  unsigned int ds;
    unsigned int fs;   unsigned int gs;  unsigned int ldt;
    unsigned short trap;
    unsigned short iomap_base;
} __attribute__((packed));

struct gdt_entry {
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char  base_middle;
    unsigned char  access;
    unsigned char  granularity;
    unsigned char  base_high;
} __attribute__((packed));

struct gdt_ptr {
    unsigned short limit;
    unsigned int   base;
} __attribute__((packed));

void gdt_install();
void gdt_set_gate(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char gran);

#endif
