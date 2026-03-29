FrodOS is an Operating System that was created for fun.

I made this because I wanted to learn how a bootloader loads a kernel and shell but I came to the conclusion that it needed more than just two simple things

How to compile this OS:

  Use a compiler that compiles C (gcc will work). This cannot use anything like m32 or freestanding as it will include the weird stuff that your computer will not recognize

  A linker to link headers (use "ld -m elf_i386 -T linker.ld boot.o kernel.o -o kernel.bin") to link the object files together

  Something to copy it (use "cp kernel.bin iso/boot/kernel.bin") to copy the files necessary

  Lastly you then use grub-mkrescue to take these files and put it together into a ISO (use "grub-mkrescue -o FrodOS.iso iso")

That is about it. Since this project isn't fully finished it has limited support so use a virtual machine like QEMU or VirtualBox for the best compatibility

Frod. Have fun with it
