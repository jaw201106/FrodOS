# 1. Variables
CC = gcc
AS = nasm
LD = ld

COMMON_FLAGS = -m32 -ffreestanding -O2 -Wall -Wextra \
               -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables
CFLAGS      = $(COMMON_FLAGS) -MMD -MP
USER_CFLAGS = $(COMMON_FLAGS) -MMD -MP
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld

# Relative path pointing straight to your new SDK toolchain project folder
SDK_DIR = ./frodos-sdk

# Bounded capacity increased to 128 MB (128 * 1024 * 1024) to match USER_MAX_IMAGE in kernel.c
USER_MAX = 134217728

OBJS = boot.o kernel.o video.o gdt.o idt.o pmm.o frodfs.o \
       speaker.o timer.o keyboard.o user_jump.o paging.o

# 2. Default target
all: FrodOS.iso

# 3. Kernel
kernel.bin: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

boot.o: boot.s
	$(AS) $(ASFLAGS) $< -o $@

user_jump.o: user_jump.s
	$(AS) $(ASFLAGS) $< -o $@

# 4. Userland shell: linked with user.ld so the flat binary starts at main
shell.o: shell.c
	$(CC) $(USER_CFLAGS) -c $< -o $@

shell.elf: shell.o user.ld
	$(LD) -m elf_i386 -T user.ld -o $@ shell.o

shell.bin: shell.elf
	objcopy -O binary $< $@
	@size=$$(stat -c %s $@); echo "shell.bin: $$size bytes"; \
	if [ $$size -gt $(USER_MAX) ]; then \
	    echo "ERROR: shell.bin too large ($$size bytes); check 'objdump -h shell.elf'"; \
	    rm -f $@; exit 1; \
	fi

# 5. RAMFS archive
# Automatically bundles your new structured application modules into your boot disk archive
ramfs.tar: shell.bin
	tar --format=ustar -cf $@ shell.bin -C $(SDK_DIR)/sys_binaries matrix.fxe

# 6. ISO
FrodOS.iso: kernel.bin ramfs.tar
	@mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/kernel.bin
	cp ramfs.tar iso/boot/ramfs.tar
	@echo 'menuentry "FrodOS" {' > iso/boot/grub/grub.cfg
	@echo '    multiboot /boot/kernel.bin' >> iso/boot/grub/grub.cfg
	@echo '    module /boot/ramfs.tar' >> iso/boot/grub/grub.cfg
	@echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ iso/

# 7. Optional: bare (unpartitioned) 64MB FAT32 test disk.
disk.img:
	dd if=/dev/zero of=$@ bs=1M count=64
	mkfs.fat -F 32 $@
	@echo "Add files with: mcopy -i disk.img somefile ::"

# 8. Run in QEMU: Added memory allocation mapping flags (-m 256M)
run: FrodOS.iso disk.img
	qemu-system-i386 -m 256M -cdrom FrodOS.iso -hda disk.img -boot d

clean:
	rm -f *.o *.d kernel.bin shell.elf shell.bin ramfs.tar FrodOS.iso
	rm -rf iso/

-include $(OBJS:.o=.d) shell.d

.PHONY: all clean run
