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
echo "New commands: echo, date, whoami, pwd, mkdir, touch, cat, nano"
