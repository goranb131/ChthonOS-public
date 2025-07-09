/* context_switch.S */

    .section .text
    .align  2

    .global context_switch
    .type   context_switch, @function

// void context_switch(context_t *old_ctx, context_t *new_ctx);
context_switch:

    cbz     x0, 1f
        stp     x19, x20, [x0, #0]      // offset 0
        stp     x21, x22, [x0, #16]     // offset 16
        stp     x23, x24, [x0, #32]
        stp     x25, x26, [x0, #48]
        stp     x27, x28, [x0, #64]
        stp     x29, x30, [x0, #80]     // x29=FP, x30=LR
        mov     x2, sp
        str     x2, [x0, #96]           
        mrs     x2, daif
        str     x2, [x0, #104]          
1:

    // restore DAIF 
    ldr     x2, [x1, #104]
    msr     daif, x2

    // restore SP
    ldr     x2, [x1, #96]
    mov     sp, x2

    // restore saved regs
    ldp     x29, x30, [x1, #80]
    ldp     x27, x28, [x1, #64]
    ldp     x25, x26, [x1, #48]
    ldp     x23, x24, [x1, #32]
    ldp     x21, x22, [x1, #16]
    ldp     x19, x20, [x1, #0]

    ret