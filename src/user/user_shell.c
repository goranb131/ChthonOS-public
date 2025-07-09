/* userspace shell to be loaded as first program - not usd currently, no C runtime */

// syscall numbers
#define SYS_PUTC    1
#define SYS_GETC    2
#define SYS_PUTS    3

// syscall wrapper functions
static inline void putc(char c) {
    asm volatile(
        "mov x8, %0\n"
        "mov x0, %1\n"
        "svc #0\n"
        :
        : "i"(SYS_PUTC), "r"((long)c)
        : "x0", "x8"
    );
}

static inline char getc(void) {
    long result;
    asm volatile(
        "mov x8, %1\n"
        "svc #0\n"
        "mov %0, x0\n"
        : "=r"(result)
        : "i"(SYS_GETC)
        : "x0", "x8"
    );
    return (char)result;
}

static inline void puts(const char *str) {
    asm volatile(
        "mov x8, %0\n"
        "mov x0, %1\n"
        "svc #0\n"
        :
        : "i"(SYS_PUTS), "r"(str)
        : "x0", "x8"
    );
}

static int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a - *b;
}

static int strncmp(const char *a, const char *b, int n) {
    while (n-- && *a && *a == *b) {
        a++;
        b++;
    }
    return n < 0 ? 0 : *a - *b;
}

static int strlen(const char *str) {
    int len = 0;
    while (*str++) len++;
    return len;
}

static void cmd_echo(const char *args) {
    if (*args) {
        puts(args);
    }
    putc('\n');
}

static void cmd_version(void) {
    puts("ChthonOS v0.2.0 \"Abyss\"\n");
    puts("Architecture: aarch64\n");
    puts("Features: Plan9-IPC, EL0-Shell, AbyssFS\n");
    puts("Build: Development\n");
}

static void cmd_clear(void) {
    puts("\033[2J\033[H");
}

static void cmd_help(void) {
    puts("Available commands:\n");
    puts("  echo <text>  - Echo text\n");
    puts("  version      - Show OS version\n");
    puts("  clear        - Clear screen\n");
    puts("  help         - Show this help\n");
}

static void execute_command(const char *cmdline) {
    while (*cmdline == ' ') cmdline++;
    
    if (*cmdline == '\0') {
        return; 
    }
    
    const char *args = cmdline;
    while (*args && *args != ' ') args++;
    int cmd_len = args - cmdline;
    
    while (*args == ' ') args++;
    
    // match commands
    if (strncmp(cmdline, "echo", 4) == 0 && (cmd_len == 4)) {
        cmd_echo(args);
    } else if (strncmp(cmdline, "version", 7) == 0 && (cmd_len == 7)) {
        cmd_version();
    } else if (strncmp(cmdline, "clear", 5) == 0 && (cmd_len == 5)) {
        cmd_clear();
    } else if (strncmp(cmdline, "help", 4) == 0 && (cmd_len == 4)) {
        cmd_help();
    } else {
        puts("Unknown command\n");
    }
}

void _user_shell_start(void) {
    char buffer[64];
    int pos = 0;
    
    while (1) {
        puts("$ ");
        
        pos = 0;
        while (1) {
            char c = getc();
            
            if (c == '\r' || c == '\n') {
                putc('\n');
                buffer[pos] = '\0';
                break;
            }
            
            if (c == 8 || c == 127) {
                if (pos > 0) {
                    pos--;
                    puts("\b \b");  // backspace, space, backspace
                }
                continue;
            }
            
            // normal characters
            if (pos < sizeof(buffer) - 1) {
                buffer[pos++] = c;
                putc(c);  // echo character
            }
        }
        
        // execute command
        execute_command(buffer);
    }
}

// required for linker
void _user_shell_end(void) {
    // just a marker for the linker
}
