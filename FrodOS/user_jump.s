[BITS 32]

global jump_to_user_mode
global syscall_asm_handler
extern syscall_handler_main

jump_to_user_mode:
    ; Retrieve user space entry point address from stack parameter
    mov ebx, [esp + 4]

    ; Disable interrupts briefly while shuffling segment registers
    cli

    ; Setup the segment registers for User Mode Data (Selector 0x20 | RPL 3 = 0x23)
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Formulate the exact x86 hardware IRET stack frame layout manually
    push 0x23           ; SS (User Data Segment Selector)
    push 0x300000       ; ESP (Temporary user stack placed safely at 3MB area)

    pushfd              ; Push current EFLAGS layout
    pop eax
    or eax, 0x200       ; Force IF (Interrupt Flag) active so shell can receive inputs
    push eax            ; EFLAGS

    push 0x1B           ; CS (User Code Segment Selector 0x18 | RPL 3 = 0x1B)
    push ebx            ; EIP (Target memory offset address of shell binary)

    ; Execute Interrupt Return. Processor pops the forged frame and drops to Ring 3!
    iretd

syscall_asm_handler:
    cli                 ; Disable interrupts while handling system call routing
    push ds             ; Save current user-space segment data context boundaries
    push es
    push fs
    push gs
    pushad              ; Save general purpose registers (EAX, ECX, EDX, EBX, etc.)

    ; Switch to standard kernel-level data segments (Index 2 -> 0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax

    ; --- FIXED: Pass the exact address of the pushad stack frame layout into C ---
    ; Because pushad was the last execution item, ESP points right to EDI, ESI... EAX.
    push esp

    call syscall_handler_main

    add esp, 4         ; Clean the single stack pointer argument off the workspace stack

    popad               ; Restore user context general purpose registers
    pop gs
    pop fs
    pop es
    pop ds
    iretd               ; Return smoothly back to User Space (Ring 3)!
