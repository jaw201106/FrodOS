; boot.s
MBOOT_PAGE_ALIGN    equ 1<<0
MBOOT_MEM_INFO      equ 1<<1
MBOOT_HEADER_MAGIC  equ 0x1BADB002
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

[BITS 32]

[GLOBAL mboot]
[EXTERN code]
[EXTERN bss]
[EXTERN end]

mboot:
    dd  MBOOT_HEADER_MAGIC
    dd  MBOOT_HEADER_FLAGS
    dd  MBOOT_CHECKSUM

[GLOBAL _start]
[GLOBAL gdt_flush]    ; Fixes gdt.c "undefined reference"
[GLOBAL idt_load]     ; Fixes idt.c "undefined reference"

[EXTERN kernel_main]
[EXTERN gp]           ; From gdt.c
[EXTERN idtp]         ; From idt.c

_start:
    ; Multiboot standard:
    ; EAX = 0x2BADB002 (Magic value)
    ; EBX = Pointer to the Multiboot information structure
    push eax            ; Second argument: magic
    push ebx            ; First argument: mboot_ptr

    call kernel_main    ; Jump into your C kernel

    ; Safety: If kernel_main returns, halt
    cli
.hang:
    hlt
    jmp .hang

; --- GDT FLUSH ---
gdt_flush:
    lgdt [gp]        ; Load the GDT pointer from gdt.c
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:flush2
flush2:
    ret

; --- IDT LOAD ---
idt_load:
    lidt [idtp]      ; Load the IDT pointer from idt.c
    ret
