#ifndef SHELL_H
#define SHELL_H

// Drivers
void kclear();
void kprint(const char* str);
void kprint_int(int n);

// Shell
void launch_shell(void* mboot_ptr);        // Add parameter here
void process_command(char* buffer, void* mboot_ptr); // Add parameter here
int str_compare(char* s1, char* s2);

// IDT
void idt_install();
// COLOR
void kset_color(unsigned char color);
// RAYCASTER
void start_raycaster();


#endif
