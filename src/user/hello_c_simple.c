/* minimal C test for userspace */

// syscall numbers
#define SYS_PUTC    1
#define SYS_TEST    5

// minimal syscall wrappers
static inline void putc(char c) {
    asm volatile(
        "mov x8, %0\n"
        "mov x0, %1\n"
        "svc #0\n"
        :
        : "i"(SYS_PUTC), "r"((long)c)
        : "x0", "x8", "memory"
    );
}

static inline long test_syscall(long input) {
    long result;
    asm volatile(
        "mov x8, %1\n"
        "mov x0, %2\n"
        "svc #0\n"
        "mov %0, x0\n"
        : "=r"(result)
        : "i"(SYS_TEST), "r"(input)
        : "x0", "x8", "memory"
    );
    return result;
}

void _start(void) {
    // test basic putc
    putc('H');
    putc('i');
    putc('!');
    putc('\n');
    
    // test syscall
    long result = test_syscall(42);
    
    // prnt result digit by digit 
    if (result >= 10) {
        putc('0' + (result / 10));
    }
    putc('0' + (result % 10));
    putc('\n');
    
    // infinite loop to prevent returning
    while(1) {
        asm volatile("wfi");  // wait for interrupt
    }
}
