    .section .user, "ax"
    .global  _user_start

_user_start:
    // Test syscall first - just output a character
    mov     x8, #1        // SYS_PUTC
    mov     x0, #'A'      // char
    svc     #0            // syscall
    
    // Test another character
    mov     x8, #1        // SYS_PUTC
    mov     x0, #'B'      // char
    svc     #0            // syscall
    
    // Test string output
    mov     x8, #3        // SYS_PUTS
    adr     x0, test_msg  // string
    svc     #0            // syscall
    
    // Loop forever
1:  wfi
    b       1b

// Data section
    .section .user, "ax"
test_msg:
    .asciz "Hello from EL0 syscall!\n"