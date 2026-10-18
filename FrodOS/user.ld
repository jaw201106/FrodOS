/* user.ld - Linker Script for FrodOS Userland Shell Application */
OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)

MEMORY
{
    /* Expanded the virtual memory segment block space from 1M to 128M */
    ram (wx) : ORIGIN = 0x200000, LENGTH = 128M
}

SECTIONS
{
    /* Enforce that our entry point layout block sits at the very beginning of the flat binary */
    .text : {
        *(.text.main)
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
    } > ram

    .data : {
        *(.data)
        *(.data.*)
    } > ram

    .bss : {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    } > ram

    /* Discard unnecessary debugging and alignment information to prevent binary bloat */
    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note.*)
    }
}
