FrodOS is an Operating System that was created for fun.

I made this because I wanted to learn how a bootloader loads a kernel and shell but I came to the conclusion that it needed more than just two simple things

How to compile this OS:


  use make to compile SDK first before beginning the rest of these steps
  
  NASM compiler is needed to make sure that boot.s will.. BOOT, so you would use a command like this (use "nasm -f elf32 boot.s -o boot.o")
  
  Use a compiler that compiles C (gcc will work). This has to use flags like m32 or freestanding as it will include the weird stuff that your computer will not recognize. Or most definitely, use a cross-compiler but this may affect **make**

  A linker to link headers (use "ld -m elf_i386 -T linker.ld boot.o kernel.o -o kernel.bin") to link the object files together

  Something to copy it (use "cp kernel.bin iso/boot/kernel.bin") to copy the files necessary

  Lastly you then use grub-mkrescue to take these files and put it together into a ISO (use "grub-mkrescue -o FrodOS.iso iso")

That is about it. Since this project isn't fully finished it has limited support so use a virtual machine like QEMU or VirtualBox for the best compatibility

Since update 0.06. I have added Makefiles! All you have to do is (use: make) and that helps compile the OS (For Linux users of course)

Building an app

There is now system APIs to use and a software development kit to create FXE (FrodOS Executable) formatted binaries. You can compile code and applications now with the release of 0.11.

Want to create an app?:

Very easy! You need to go into apps and review elements that you can include,

("int make_syscall(int num, unsigned int arg1);
void sys_print(const char* s);
void sys_yield(void);
char sys_read_char(void);
void sys_clear(void);
void sys_delay(int loops);
void sys_exit(void);")

these mostly define most of the library as its just syscalls but each can be applied to the code. MAKE SURE YOU CODE IN C!

When all of that is done, compiling is usually the easy part. If you plan to run this in the OS, then make sure the SDK is in the root folder where the source code lies, make sure you have changed what the Makefile is targeting in both folders of FrodOS and frodos-sdk. run 'make' in both folders and run the OS, it should now be there.


