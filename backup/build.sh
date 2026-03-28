#!/bin/bash

# Check if NASM is installed
if ! command -v nasm &> /dev/null; then
    echo "Error: NASM is required but not installed."
    exit 1
fi

echo "Building FrodOS..."
echo "Using NASM version: $(nasm -v | head -1)"

# Clean old files
rm -f *.o kernel.bin
rm -rf iso
rm -f frodos.iso

# Assemble boot sector
echo "Assembling boot.s..."
nasm -f elf32 boot.s -o boot.o
if [ $? -ne 0 ]; then echo "NASM failed!"; exit 1; fi

# Compile all C files
echo "Compiling C files..."
for cfile in *.c; do
    echo "  Compiling $cfile..."
    gcc -m32 -ffreestanding -c $cfile -o ${cfile%.c}.o
    if [ $? -ne 0 ]; then echo "Compilation failed for $cfile!"; exit 1; fi
done

# Link everything
echo "Linking..."
ld -m elf_i386 -T linker.ld -o kernel.bin *.o
if [ $? -ne 0 ]; then echo "Linking failed!"; exit 1; fi

# Check multiboot header
echo "Checking multiboot header..."
if hexdump -C kernel.bin | grep -q "1b ad b0 02"; then
    echo "✓ Multiboot header found!"
else
    echo "✗ Multiboot header NOT found!"
    exit 1
fi

# Create ISO structure
echo "Creating ISO..."
mkdir -p iso/boot/grub
cp kernel.bin iso/boot/
cp grub.cfg iso/boot/grub/

# Generate bootable ISO
grub-mkrescue -o frodos.iso iso/ 2>&1
if [ $? -ne 0 ]; then
    echo "ISO creation failed!"
    echo "Try installing: sudo apt-get install grub-pc-bin grub-common xorriso"
    exit 1
fi

echo ""
echo "=========================================="
echo "Build complete!"
echo "=========================================="
echo "Files created:"
ls -lh kernel.bin
ls -lh frodos.iso
echo ""
echo "To run in QEMU:"
echo "  qemu-system-i386 -cdrom frodos.iso"
echo "=========================================="