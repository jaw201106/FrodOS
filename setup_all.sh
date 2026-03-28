#!/bin/bash
cd ~/FrodOS

# Create boot.s
cat > boot.s << 'BOOT'
; boot.s - Fixed multiboot header
[BITS 32]

; Multiboot header
section .multiboot
align 4
    dd 0x1BADB002
    dd 0x03
    dd -(0x1BADB002 + 0x03)

; Kernel entry point
section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top
    push ebx
    call kernel_main
    cli
.hang:
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
BOOT

# Create linker.ld
cat > linker.ld << 'LD'
ENTRY(_start)

SECTIONS
{
    . = 1M;
    
    .multiboot : {
        *(.multiboot)
    }
    
    .text : {
        *(.text)
    }
    
    .data : {
        *(.data)
    }
    
    .bss : {
        *(.bss)
        *(COMMON)
    }
}
LD

# Create grub.cfg
cat > grub.cfg << 'GRUB'
menuentry "FrodOS" {
    multiboot /boot/kernel.bin
}
GRUB

# Create build.sh
cat > build.sh << 'BUILD'
#!/bin/bash
set -e

echo "Building FrodOS..."
nasm -f elf32 boot.s -o boot.o
gcc -m32 -ffreestanding -c *.c
ld -m elf_i386 -T linker.ld -o kernel.bin *.o

echo "Checking multiboot header..."
hexdump -C kernel.bin | grep -q "1b ad b0 02" && echo "✓ OK" || echo "✗ FAIL"

mkdir -p iso/boot/grub
cp kernel.bin iso/boot/
cp grub.cfg iso/boot/grub/
grub-mkrescue -o frodos.iso iso/

echo "Done! Run: qemu-system-i386 -cdrom frodos.iso"
BUILD

chmod +x build.sh

echo "Setup complete! Now run: ./build.sh"
