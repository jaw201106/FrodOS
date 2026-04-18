; boot.s
MBOOT_PAGE_ALIGN    equ 1<<0
MBOOT_MEM_INFO      equ 1<<1
MBOOT_HEADER_MAGIC  equ 0x1BADB002
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

[BITS 32]

; Put the header back at the top without a section tag
; so it stays at the very front of the binary.
mboot:
    dd  MBOOT_HEADER_MAGIC
    dd  MBOOT_HEADER_FLAGS
    dd  MBOOT_CHECKSUM

[GLOBAL _start]
[GLOBAL gdt_flush]
[GLOBAL idt_load]
[GLOBAL irq1_handler]

[EXTERN kernel_main]
[EXTERN keyboard_handler_main]
[EXTERN gp]
[EXTERN idtp]

_start:
    ; --- THE FIX: SETUP THE STACK ---
    mov esp, stack_top

    push eax
    push ebx

    call kernel_main

    cli
.hang:
    hlt
    jmp .hang

; --- GDT FLUSH ---
gdt_flush:
    lgdt [gp]
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
    lidt [idtp]
    ret

; --- KEYBOARD INTERRUPT HANDLER ---
irq1_handler:
    pushad
    cld
    call keyboard_handler_main
    popad
    iretd

; --- RESERVED STACK SPACE ---
section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
