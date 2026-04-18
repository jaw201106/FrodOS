#ifndef SHELL_H
#define SHELL_H

// Types (Defining these here ensures every file has them)
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

// Video/Display
void kclear();
void kprint(const char* str);
void kprint_int(int n);
void put_char(char c);             // <--- ADD THIS
void kset_color(unsigned char color);
void update_cursor();              // <--- ADD THIS

// Shell Logic
void launch_shell(void* mboot_ptr);
void process_command(char* buffer, void* mboot_ptr);
int str_compare(char* s1, char* s2);
int str_compare_partial(char* s1, char* s2); // <--- ADD THIS

// IDT
void idt_install();

// Filesystem (FAT32)
void fat32_init(uint32_t partition_lba);     // <--- ADD THIS
void fat32_ls();                             // <--- ADD THIS
void fat32_cd(char* dirname);                // <--- ADD THIS

#endif
