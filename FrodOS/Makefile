# 1. Variables
CC = gcc
AS = nasm
LD = ld

# Compiler flags
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld

# Objects to link into the central Kernel Core
OBJS = boot.o kernel.o video.o gdt.o idt.o pmm.o ata.o mbr.o fat32.o speaker.o timer.o keyboard.o user_jump.o

# 3. Default target
all: FrodOS.iso

# 4. Link the kernel binary
kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# 5. Compile standard object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 6. Build the standalone command parser application (Userland entry)
# FIXED: Base-shifted text space to 0x200000 to match the kernel deployment zone layout
shell.bin: shell.c
	$(CC) $(CFLAGS) -static -nostdlib -e main -Wl,-Ttext=0x200000 $< -o shell.elf
	objcopy -O binary shell.elf shell.bin


# 7. Package the compiled application into a standard read-only TAR archive
ramfs.tar: shell.bin
	tar --format=ustar -cf ramfs.tar shell.bin

# 8. Assemble the bootloader
boot.o: boot.s
	$(AS) $(ASFLAGS) $< -o $@

user_jump.o: user_jump.s
	$(AS) $(ASFLAGS) $< -o $@

# 9. Create the ISO and pack both the kernel and RAMFS together
FrodOS.iso: kernel.bin ramfs.tar
	@mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/kernel.bin
	cp ramfs.tar iso/boot/ramfs.tar
	@echo 'menuentry "FrodOS" {' > iso/boot/grub/grub.cfg
	@echo '    multiboot /boot/kernel.bin' >> iso/boot/grub/grub.cfg
	@echo '    module /boot/ramfs.tar' >> iso/boot/grub/grub.cfg
	@echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue -o FrodOS.iso iso/

# 10. Cleanup routine updates
clean:
	rm -f *.o kernel.bin shell.bin ramfs.tar FrodOS.iso
	rm -rf iso/

.PHONY: all clean
