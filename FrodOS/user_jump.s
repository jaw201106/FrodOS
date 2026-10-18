[BITS 32]

global jump_to_user_mode
global syscall_asm_handler
extern syscall_handler_main

USER_CODE_SEL equ 0x1B      ; GDT index 3 | RPL 3
USER_DATA_SEL equ 0x23      ; GDT index 4 | RPL 3
KERN_DATA_SEL equ 0x10
USER_STACK    equ 0x300000  ; top of the user stack (grows down)

; void jump_to_user_mode(uint32_t entry)
jump_to_user_mode:
    mov ebx, [esp + 4]
    cli

    mov ax, USER_DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push USER_DATA_SEL      ; SS
    push USER_STACK         ; ESP
    pushfd
    pop eax
    or  eax, 0x200          ; IF=1 so the shell receives interrupts
    push eax                ; EFLAGS
    push USER_CODE_SEL      ; CS
    push ebx                ; EIP
    iretd

; int 0x80 entry. The IDT gate for 0x80 must have DPL 3 (attribute 0xEE).
syscall_asm_handler:
    cli
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

    push esp                ; syscall_regs_t* (edi, esi, ebp, esp, ebx, edx, ecx, eax)
    call syscall_handler_main
    add esp, 4

    popad
    pop gs
    pop fs
    pop es
    pop ds
    iretd
