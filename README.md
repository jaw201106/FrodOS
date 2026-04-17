FrodOS is an Operating System that was created for fun.

I made this because I wanted to learn how a bootloader loads a kernel and shell but I came to the conclusion that it needed more than just two simple things

How to compile this OS:

  NASM compiler is needed to make sure that boot.s will.. BOOT, so you would use a command like this (use "nasm -f elf32 boot.s -o boot.o")
  
  Use a compiler that compiles C (gcc will work). This has to use flags like m32 or freestanding as it will include the weird stuff that your computer will not recognize

  A linker to link headers (use "ld -m elf_i386 -T linker.ld boot.o kernel.o -o kernel.bin") to link the object files together

  Something to copy it (use "cp kernel.bin iso/boot/kernel.bin") to copy the files necessary

  Lastly you then use grub-mkrescue to take these files and put it together into a ISO (use "grub-mkrescue -o FrodOS.iso iso")

That is about it. Since this project isn't fully finished it has limited support so use a virtual machine like QEMU or VirtualBox for the best compatibility

BUUUTT Since update 0.06. I have added Makefiles! All you have to do is (use: make) and that helps compile the OS (For Linux users of course) Bust for anyone who uses any other Operating system but I'll add that later.
