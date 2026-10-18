# frodos-sdk/Makefile
CC = gcc
LD = ld
OBJCOPY = objcopy

# Cross-compilation flags targeting our 32-bit x86 Ring 3 isolated system environment
USER_CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -fno-pie -fno-stack-protector -I.

APPS_SRC = $(wildcard apps/*.c)
APPS_BIN = sys_binaries/matrix.fxe

.PHONY: all clean prep_dirs

all: prep_dirs tools/fxe_packer $(APPS_BIN)
	@echo "========================================================="
	@echo " FrodOS SDK Build Completed. FXE Binaries are in sys_binaries/"
	@echo "========================================================="

prep_dirs:
	@mkdir -p tools sys_binaries

# Compile host toolchain packer utility
tools/fxe_packer: tools/fxe_packer.c fxe.h
	gcc -O2 -Wall tools/fxe_packer.c -o tools/fxe_packer

# Rule template to generate individual FXE binaries from application source modules
sys_binaries/%.fxe: apps/%.c apps/libfrodos.c apps/user.ld tools/fxe_packer fxe.h
	# 1. Compile source assets to intermediate object states
	$(CC) $(USER_CFLAGS) -c apps/$*.c -o apps/$*.o
	$(CC) $(USER_CFLAGS) -c apps/libfrodos.c -o apps/libfrodos.o
	# 2. Link objects into unified ELF structure matching userland layout
	$(LD) -m elf_i386 -T apps/user.ld -o apps/$*.elf apps/$*.o apps/libfrodos.o
	# 3. Separate executable streams cleanly into dedicated segment layers
	$(OBJCOPY) -O binary --only-section=.text apps/$*.elf apps/$*_text.bin
	$(OBJCOPY) -O binary --only-section=.data apps/$*.elf apps/$*_data.bin
	# 4. Pack binaries under formal FXE structures into target execution containers
	./tools/fxe_packer apps/$*_text.bin apps/$*_data.bin $@
	# 5. Clean dynamic transient layout files safely
	@rm -f apps/$*.o apps/libfrodos.o apps/$*.elf apps/$*_text.bin apps/$*_data.bin

clean:
	rm -rf tools/fxe_packer sys_binaries/ apps/*.o apps/*.elf apps/*.bin
