// shell.c (Pure User-Space Application)
typedef __SIZE_TYPE__ size_t;

static inline int make_syscall(int num, unsigned int arg1) {
    int retval;
    // Explicitly binding EAX and EBX to force the compiler layout compliance
    __asm__ volatile (
        "int $0x80"
        : "=a"(retval)
        : "0"(num), "b"(arg1)
        : "memory"
    );
    return retval;
}

void sys_print(const char* str) {
    make_syscall(1, (unsigned int)str); // System call 1 = Print String
}

void sys_yield(void) {
    make_syscall(2, 0);                 // System call 2 = Yield / Halt
}

char sys_read_char(void) {
    return (char)make_syscall(3, 0);    // System call 3 = Non-blocking key fetch
}

// Extends support handlers for extra services mapped to kernel.c
void sys_clear(void) { make_syscall(4, 0); }
void sys_ls(void)    { make_syscall(5, 0); }
void sys_cat(const char* file) { make_syscall(6, (unsigned int)file); }

// Tiny user-space string comparison helper
static int user_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void main(void) {
    sys_clear(); // Clean boot console presentation baseline
    sys_print("========================================\n");
    sys_print("         Welcome to FrodOS Userland!      \n");
    sys_print("========================================\n\n");
    sys_print("FrodOS $ ");

    char command_buffer[256];
    int buf_idx = 0;

    // Active terminal entry processing loop framework
    while(1) {
        char c = sys_read_char();

        if (c == 0) {
            // No character waiting in queue? Yield processor to lower execution overhead
            sys_yield();
            continue;
        }

        if (c == '\n') {
            command_buffer[buf_idx] = '\0';
            sys_print("\n"); // Push to next text block baseline line before processing

            if (buf_idx > 0) {
                // --- CENTRAL CORE USER SPACE COMMAND PARSER ---
                if (user_strcmp(command_buffer, "help") == 0) {
                    sys_print("Available commands:\n");
                    sys_print("  help  - Display this menu list options\n");
                    sys_print("  clear - Clears the terminal screen\n");
                }
                else if (user_strcmp(command_buffer, "clear") == 0) {
                        sys_clear();
                }
                else {
                    sys_print("Unknown command: ");
                    sys_print(command_buffer);
                    sys_print("\nType 'help' to review configuration commands.");
                }
            }

            sys_print("\nFrodOS $ ");
            buf_idx = 0;
        }
        // --- FIXED: Protected Backspace Privilege Boundaries ---
        else if (c == '\b') {
            if (buf_idx > 0) {
                buf_idx--;
                // Explicitly send a visual destructive backspace configuration stack out to video.c
                // Backtrack cursor position -> print space character -> backtrack cursor position again
                sys_print("\b \b");
            }
            // If buf_idx == 0, we simply drop the execution block, safely masking the prompt text string!
        }
        else {
            if (buf_idx < 255) {
                command_buffer[buf_idx++] = c;

                // Explicitly print individual incoming character string values back down to text layers
                char echo_str[2] = {c, '\0'};
                sys_print(echo_str);
            }
        }
    }
}
