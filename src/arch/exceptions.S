// exceptions.S – EL1 vector table + print
    .section .text
    .align   11                       // 2 KiB alignment (QEMU requirement)
    .global  exception_vector_table
exception_vector_table:

/*  EL1 (SP0)  */
    .align 7 ;  b sync_handler        
    .align 7 ;  b .                   
    .align 7 ;  b .                   
    .align 7 ;  b .                   

/* EL1 (SPx) */
    .align 7 ;  b sync_handler
    .align 7 ;  b curr_el_irq
    .align 7 ;  b .
    .align 7 ;  b .

/* lower EL (EL0)  */
    .align 7 ;  b sync_handler
    .align 7 ;  b curr_el_irq
    .align 7 ;  b .
    .align 7 ;  b .

/* lower EL AArch32 unused */
    .align 7 ;  b .
    .align 7 ;  b .
    .align 7 ;  b .
    .align 7 ;  b .


sync_handler:
    sub     sp, sp, #80           // space for x0-x9
    stp     x0, x1, [sp]          // save x0, x1
    stp     x2, x3, [sp,#16]      // save x2, x3
    stp     x4, x5, [sp,#32]      // save x4, x5
    stp     x6, x7, [sp,#48]      // save x6, x7
    stp     x8, x9, [sp,#64]      // save x8, x9

    // dont dump ESR/FAR for normal operation
    // mrs     x0, esr_el1          
    // mrs     x1, far_el1         
    // bl      uart_dump_esr_far   

    // pass saved register values to C handler
    ldp     x0, x1, [sp]          // load saved x0, x1 as args 0,1
    ldp     x2, x3, [sp,#16]      // load saved x2, x3 as args 2,3
    // need to load saved x8 into x3 (4th argument)
    ldr     x3, [sp,#64]          // load saved x8 as 4th argument
    bl      handle_sync_exception
    
    // check. bits 63:32 = 0 for syscall, 1 for halt
    // for syscalls bits 31:0 = return value for users x0
    mov     x1, x0                // save full return value
    lsr     x2, x0, #32           // extract upper 32 bits
    cmp     x2, #0                // check if upper bits are 0 (syscall) or 1 (halt)
    bne     halt_system           // if non-zero then halt
    
    // normal return path for syscalls
    // store syscall return value in users x0 register
    str     x1, [sp]              // store return value in saved x0 slot
    
    ldp     x8, x9, [sp,#64]      // restore x8, x9
    ldp     x6, x7, [sp,#48]      // restore x6, x7
    ldp     x4, x5, [sp,#32]      // restore x4, x5
    ldp     x2, x3, [sp,#16]      // restore x2, x3
    ldp     x0, x1, [sp]          // restore x0 (with syscall result), x1
    add     sp, sp, #80
    eret

halt_system:
    // disable all interrupts
    msr     daifset, #15
    // infinite loop with WFI
1:  wfi
    b       1b

curr_el_irq:
    sub     sp, sp, #16
    stp     x30, xzr, [sp]       
    bl      handle_irq
    ldp     x30, xzr, [sp]
    add     sp, sp, #16
    eret

    .global uart_dump_esr_far
    .global uart_dump_esr_far
uart_dump_esr_far:
    // x0 = ESR_EL1, x1 = FAR_EL1
    stp     x30, x19, [sp,#-16]!  // save lr and x19
    mov     x19, x0               // save ESR value
    
    adr     x2, dump_msg
1:  ldrb    w3, [x2], #1
    cbz     w3, 2f
    mov     x0, x3                // move character to x0 for uart_putc
    bl      uart_putc
    b       1b
2:  mov     x0, x19               // ESR value for uart_hex
    bl      uart_hex
    mov     x0, #' '
    bl      uart_putc
    mov     x0, x1                // FAR value for uart_hex
    bl      uart_hex
    mov     x0, #'\n'
    bl      uart_putc
    
    ldp     x30, x19, [sp], #16   // restore lr and x19
    ret

dump_msg: .asciz "SYNC: "


    .global handle_sync_exception

