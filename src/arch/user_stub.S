/* EL0 syscall test */
    .section .user, "ax"
    .global  _user_start

_user_start:
    // test syscall, just output a character
    mov     x8, #1        // SYS_PUTC
    mov     x0, #'A'      // char to print
    svc     #0            // make syscall
    
    // test another character
    mov     x8, #1        
    mov     x0, #'B'      
    svc     #0            
    
    // test string output
    mov     x8, #3        
    adr     x0, test_msg  
    svc     #0            
    
    // Loop forever
1:  wfi
    b       1b

// data section
    .section .user, "ax"
test_msg:
    .asciz "Hello from EL0 syscall!\n"