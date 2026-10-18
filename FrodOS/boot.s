; boot.s
MBOOT_PAGE_ALIGN    equ 1<<0
MBOOT_MEM_INFO      equ 1<<1
MBOOT_HEADER_MAGIC  equ 0x1BADB002
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

KERN_DATA_SEL       equ 0x10

[BITS 32]

section .multiboot
align 4
mboot:
    dd  MBOOT_HEADER_MAGIC
    dd  MBOOT_HEADER_FLAGS
    dd  MBOOT_CHECKSUM

section .text

[GLOBAL _start]
[GLOBAL gdt_flush]
[GLOBAL idt_load]
[GLOBAL irq0_handler]
[GLOBAL irq1_handler]
[GLOBAL exception13_handler]

[EXTERN kernel_main]
[EXTERN timer_callback]
[EXTERN keyboard_handler_main]
[EXTERN gpf_handler_main]
[EXTERN gp]
[EXTERN idtp]

_start:
    cli
    cld
    mov esp, stack_top
    push eax                ; magic        (2nd argument)
    push ebx                ; mboot_ptr    (1st argument)
    call kernel_main
.hang:
    cli
    hlt
    jmp .hang

gdt_flush:
    lgdt [gp]
    mov ax, KERN_DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:flush2
flush2:
    ret

idt_load:
    lidt [idtp]
    ret

; Shared body for hardware IRQ stubs (master PIC only).
%macro IRQ_STUB 2           ; %1 = label, %2 = C handler
%1:
    push ds
    push es
    push fs
    push gs
    pushad
    mov ax, KERN_DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    cld
    call %2
    mov al, 0x20            ; EOI to master PIC
    out 0x20, al
    popad
    pop gs
    pop fs
    pop es
    pop ds
    iretd
%endmacro

IRQ_STUB irq0_handler, timer_callback
IRQ_STUB irq1_handler, keyboard_handler_main

; #GP: the CPU pushes an error code, then EIP, CS, EFLAGS (and ESP/SS from ring 3).
exception13_handler:
    cli
    mov eax, [esp]          ; error code
    mov ebx, [esp + 4]      ; faulting EIP
    mov ecx, [esp + 8]      ; faulting CS
    mov dx, KERN_DATA_SEL
    mov ds, dx
    mov es, dx
    mov fs, dx
    mov gs, dx
    cld
    push ecx
    push ebx
    push eax
    call gpf_handler_main   ; gpf_handler_main(err, eip, cs)
.halt_loop:
    hlt
    jmp .halt_loop

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
