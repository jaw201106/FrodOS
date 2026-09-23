ENTRY(_start)

SECTIONS
{
    /* The kernel is loaded by GRUB at 1MB physical address bounds */
    . = 0x100000;

    .text : ALIGN(4K)
    {
        *(.multiboot) /* <--- FORCE MULTIBOOT HEADER TO BE FIRST REGARDLESS OF OPTIMIZATION */
        *(.text)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    .data : ALIGN(4K)
    {
        *(.data)
    }

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    /* Provide the global tracking anchor for your Physical Memory Manager */
    _kernel_end = .;
}
