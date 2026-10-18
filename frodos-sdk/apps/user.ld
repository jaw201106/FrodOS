OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)

MEMORY
{
    ram (wx) : ORIGIN = 0x6000000, LENGTH = 32M
}

SECTIONS
{
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

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note.*)
    }
}
