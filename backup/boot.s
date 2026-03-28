; boot.s - Fixed multiboot header
[BITS 32]

; Multiboot header
section .multiboot
align 4
    dd 0x1BADB002          ; Magic number
    dd 0x03                ; Flags (ALIGN | MEMINFO)
    dd -(0x1BADB002 + 0x03) ; Checksum

; Kernel entry point
section .text
global _start
extern kernel_main

_start:
    ; Set up stack
    mov esp, stack_top
    
    ; Push multiboot info pointer
    push ebx
    
    ; Call kernel main
    call kernel_main
    
    ; Halt if kernel returns
    cli
.hang:
    hlt
    jmp .hang

; Stack section
section .bss
align 16
stack_bottom:
    resb 16384  ; 16KB stack
stack_top: