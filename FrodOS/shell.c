// shell.c (pure user-space application)
typedef __SIZE_TYPE__ size_t;

#define CMD_MAX 256
#define IO_MAX  4096

// --- Core System Call Engine ---
static inline int make_syscall(int num, unsigned int arg1) {
    int ret;
    __asm__ volatile ("int $0x80"
    : "=a"(ret)
    : "0"(num), "b"(arg1)
    : "memory");
    return ret;
}

// --- Standard System Call Wrappers ---
static void sys_print(const char* s)        { make_syscall(1, (unsigned int)s); }
static void sys_yield(void)                 { make_syscall(2, 0); }
static char sys_read_char(void)             { return (char)make_syscall(3, 0); }
static void sys_clear(void)                 { make_syscall(4, 0); }
static void sys_play_sound(unsigned int hz) { make_syscall(7, hz); }
static void sys_nosound(void)               { make_syscall(8, 0); }

/* --- Brand New FrodFS Filesystem System Call Interfaces --- */
static int sys_fs_create(const char* name) {
    return make_syscall(10, (unsigned int)name);
}

static int sys_fs_write(const char* name, const char* content, unsigned int size) {
    unsigned int args[3] = { (unsigned int)name, (unsigned int)content, size };
    return make_syscall(11, (unsigned int)args);
}

static int sys_fs_read(const char* name, char* buffer, unsigned int max, unsigned int* out_size) {
    unsigned int args[4] = { (unsigned int)name, (unsigned int)buffer, max, (unsigned int)out_size };
    return make_syscall(12, (unsigned int)args);
}

static int sys_fs_list(char* buffer, unsigned int max_bytes) {
    unsigned int args[2] = { (unsigned int)buffer, max_bytes };
    return make_syscall(13, (unsigned int)args);
}

/* --- Executable Program Launcher System Call Wrapper --- */
static int sys_exec(const char* filename) {
    return make_syscall(20, (unsigned int)filename);
}

// --- High-Performance System Delays & Beeps ---
static void sys_delay(int loops) {
    for (volatile int i = 0; i < loops; i++) { }
}

static void sys_beep(void) {
    sys_play_sound(1000);
    sys_delay(15000000);
    sys_nosound();
}

// --- Safe String Operations ---
static int user_strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

static int user_strncmp(const char* a, const char* b, size_t n) {
    while (n > 0 && *a && *a == *b) { a++; b++; n--; }
    if (n == 0) return 0;
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

// Single multi-purpose dynamic scratch space allocated outside the stack
static char io_buffer[IO_MAX];

// --- Sub-system Command Execution Logic ---
static void run_command(char* cmd) {
    // Clean up strings by stripping out any hanging whitespace/newlines
    int clen = 0;
    while (cmd[clen] != '\0') {
        if (cmd[clen] == '\r' || cmd[clen] == '\n') {
            cmd[clen] = '\0';
            break;
        }
        clen++;
    }

    if (user_strcmp(cmd, "help") == 0) {
        sys_print("Available commands:\n");
        sys_print("  help                 - Show this menu\n");
        sys_print("  clear                - Clear the screen\n");
        sys_print("  beep                 - Test the PC speaker\n");
        sys_print("  ls                   - List storage files\n");
        sys_print("  touch <file>         - Create an empty file\n");
        sys_print("  write <file> <text>  - Write message content to file\n");
        sys_print("  cat <file>           - Print contents of a file\n");
    }
    else if (user_strcmp(cmd, "clear") == 0) {
        sys_clear();
    }
    else if (user_strcmp(cmd, "beep") == 0) {
        sys_print("Sounding hardware test\n");
        sys_beep();
    }
    else if (user_strcmp(cmd, "ls") == 0) {
        if (sys_fs_list(io_buffer, sizeof(io_buffer)) == 0) {
            if (io_buffer[0] == '\0') {
                sys_print("[Empty Storage Disk]\n");
            } else {
                sys_print(io_buffer);
            }
        } else {
            sys_print("Error: Unable to list target directory structures.\n");
        }
    }
    else if (user_strncmp(cmd, "touch ", 6) == 0) {
        char* filename = cmd + 6;
        while (*filename == ' ') { filename++; }

        if (*filename == '\0') {
            sys_print("Usage: touch <filename>\n");
            return;
        }

        int res = sys_fs_create(filename);
        if (res == 0)       sys_print("File created successfully.\n");
        else if (res == -2) sys_print("Error: File descriptor already exists.\n");
        else                sys_print("Error: Out of storage capacity nodes.\n");
    }
    else if (user_strncmp(cmd, "write ", 6) == 0) {
        char* filename = cmd + 6;
        while (*filename == ' ') { filename++; }

        char* text = filename;
        while (*text != '\0' && *text != ' ') { text++; }

        if (*text == ' ') {
            *text = '\0';
            text++;
            while (*text == ' ') { text++; }

            unsigned int text_len = 0;
            while (text[text_len] != '\0') { text_len++; }

            int res = sys_fs_write(filename, text, text_len);
            if (res == 0) sys_print("Payload written to node successfully.\n");
            else          sys_print("Error: Could not save data block to entry.\n");
        } else {
            sys_print("Usage: write <filename> <text payload contents>\n");
        }
    }
    else if (user_strncmp(cmd, "cat ", 4) == 0) {
        char* filename = cmd + 4;
        while (*filename == ' ') { filename++; }

        if (*filename == '\0') {
            sys_print("Usage: cat <filename>\n");
            return;
        }

        unsigned int bytes_read = 0;
        int res = sys_fs_read(filename, io_buffer, sizeof(io_buffer) - 1, &bytes_read);

        if (res == 0) {
            io_buffer[bytes_read] = '\0';
            sys_print(io_buffer);
            sys_print("\n");
        } else {
            sys_print("Error: Target file not found or unreadable.\n");
        }
    }
    else {
        /* =================================================================== */
        /* FALLBACK: If not a built-in command, pass straight to the kernel   */
        /* =================================================================== */
        int exec_res = sys_exec(cmd);

        if (exec_res == -2) {
            sys_print("Unknown command: ");
            sys_print(cmd);
            sys_print("\nType 'help' for a list of commands.\n");
        }
        else if (exec_res == -4) {
            sys_print("Error: Target file found but it is not a valid FXE format structure.\n");
        }
        else if (exec_res < 0) {
            sys_print("Error: Executable file layout verification failed.\n");
        }
        /* =================================================================== */
    }
}

// --- Entry Point Definition Layout ---
__attribute__((section(".text.main")))
void main(void) {
    char buf[CMD_MAX];
    int len = 0;

    sys_clear();
    sys_print("========================================\n");
    sys_print("         Welcome to FrodOS Userland!      \n");
    sys_print("========================================\n\n");
    sys_print("FrodOS $ ");

    for (;;) {
        char c = sys_read_char();

        if (c == 0) {
            sys_yield();
            continue;
        }

        if (c == '\n' || c == '\r') {
            buf[len] = '\0';
            sys_print("\n");
            if (len > 0) {
                run_command(buf);
                sys_print("\n");
            }
            sys_print("FrodOS $ ");
            len = 0;
        }
        else if (c == '\b') {
            if (len > 0) {
                len--;
                sys_print("\b \b");
            }
        }
        else if (len < CMD_MAX - 1) {
            buf[len++] = c;
            char echo[2] = { c, '\0' };
            sys_print(echo);
        }
    }
}
