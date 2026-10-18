#ifndef SHELL_H
#define SHELL_H

// Core Safe freestanding types definition layer
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

// --- Video / Screen Console Functions ---
void kclear(void);
void kprint(const char* str);
void kprint_int(int n);
void put_char(char c);
void kset_color(unsigned char color);
void update_cursor(void);

// --- Core OS / Shell Flow Control ---
void launch_shell(void* mboot_ptr);
void process_command(char* buffer, void* mboot_ptr);
int str_compare(char* s1, char* s2);
int str_compare_partial(char* s1, char* s2);

// --- IDT & Interrupt Functions ---
void idt_install(void);

void kprint_hex8(uint8_t val);

#endif
