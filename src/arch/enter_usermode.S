// enter_usermode.S – minimal, relocation-free
    .text
    .global enter_usermode
    .type   enter_usermode,%function

/* void enter_usermode(uint64_t pc, uint64_t sp) */
enter_usermode:
    msr     sp_el0,  x1        // user stack
    msr     elr_el1, x0        // user PC
    
    // Clear all general purpose registers for clean user start
    mov     x0, #0
    mov     x1, #0
    mov     x2, #0
    mov     x3, #0
    mov     x4, #0
    mov     x5, #0
    mov     x6, #0
    mov     x7, #0
    mov     x8, #0
    mov     x9, #0
    mov     x10, #0
    mov     x11, #0
    mov     x12, #0
    mov     x13, #0
    mov     x14, #0
    mov     x15, #0
    mov     x16, #0
    mov     x17, #0
    mov     x18, #0
    mov     x19, #0
    mov     x20, #0
    mov     x21, #0
    mov     x22, #0
    mov     x23, #0
    mov     x24, #0
    mov     x25, #0
    mov     x26, #0
    mov     x27, #0
    mov     x28, #0
    mov     x29, #0  // Frame pointer
    mov     x30, #0  // Link register
    
    // disable interrupts in user mode for shell
    mov     x2, #(1 << 7) // SPSR = EL0t, IRQs disabled (I=1)
    msr     spsr_el1, x2
    isb
    eret                       // EL0, never returns
    
    // should never reach here
    b       .