; boot.s
MBOOT_PAGE_ALIGN    equ 1<<0
MBOOT_MEM_INFO      equ 1<<1
MBOOT_HEADER_MAGIC  equ 0x1BADB002
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

[BITS 32]

; Force the header into a dedicated section that the linker script can grab
section .multiboot
align 4
mboot:
    dd  MBOOT_HEADER_MAGIC
    dd  MBOOT_HEADER_FLAGS
    dd  MBOOT_CHECKSUM

; Switch to the main executable text section
section .text

[GLOBAL _start]
[GLOBAL gdt_flush]
[GLOBAL idt_load]
[GLOBAL irq0_handler]         ; Exposed to hook the PIT clock ticker
[GLOBAL irq1_handler]         ; Exposed to hook the keyboard
[GLOBAL exception13_handler]  ; Exposed to hook General Protection Faults

[EXTERN kernel_main]
[EXTERN timer_callback]       ; From timer.c
[EXTERN keyboard_handler_main]; From your keyboard driver
[EXTERN gpf_handler_main]     ; From kernel.c
[EXTERN gp]
[EXTERN idtp]

_start:
    ; 1. Secure an explicit 16KB execution stack boundary
    mov esp, stack_top

    ; 2. GRUB parameters: push EAX (magic) first, then EBX (mboot_ptr) last
    ; This ensures C reads (void* mboot_ptr, unsigned int magic) in correct order
    push eax
    push ebx

    call kernel_main

    ; Safety dead loop fallback trap
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

; --- SYSTEM TIMER INTERRUPT HANDLER (IRQ 0) ---
irq0_handler:
    pushad
    cld
    call timer_callback

    ; Send End-Of-Interrupt (EOI) signal byte to the Master PIC
    mov al, 0x20
    out 0x20, al

    popad
    iretd

; --- KEYBOARD INTERRUPT HANDLER (IRQ 1) ---
irq1_handler:
    pushad
    cld
    call keyboard_handler_main

    ; Send End-Of-Interrupt (EOI) signal byte to the Master PIC
    mov al, 0x20
    out 0x20, al

    popad
    iretd

; --- GENERAL PROTECTION FAULT TRAP HANDLER (Exception 13) ---
exception13_handler:
    ; Note: CPU pushes an error code onto the stack for Exception 13 automatically.
    ; We can clear interrupts or hop straight into our crash reporter display.
    cli
    call gpf_handler_main
.halt_loop:
    hlt
    jmp .halt_loop

; --- RESERVED STACK SPACE ---
section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
