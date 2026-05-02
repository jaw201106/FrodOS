# 1. Variables
CC = gcc
AS = nasm
LD = ld

# Compiler flags
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld

# 2. Objects to link - ADDED ata.o HERE
OBJS = boot.o kernel.o video.o gdt.o idt.o shell.o pmm.o ata.o mbr.o fat32.o speaker.o

# 3. Default target
all: FrodOS.iso

# 4. Link the kernel binary
kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# 5. Compile C files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 6. Assemble the bootloader
boot.o: boot.s
	$(AS) $(ASFLAGS) $< -o $@

# 7. Create the ISO
FrodOS.iso: kernel.bin
	@mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/kernel.bin
	@touch iso/boot/kernel.bin
	grub-mkrescue -o FrodOS.iso iso/

# 8. Cleanup
clean:
	rm -f *.o kernel.bin FrodOS.iso
	rm -f iso/boot/kernel.bin

.PHONY: all clean
