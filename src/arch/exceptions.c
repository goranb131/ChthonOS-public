#ifndef _EXCEPTIONS_C
#define _EXCEPTIONS_C

#include "uart.h"
#include "exceptions.h"
#include "message.h"

typedef unsigned long uint64_t;

// syscall numbers
#define SYS_PUTC    1
#define SYS_GETC    2
#define SYS_PUTS    3
#define SYS_SEND_MESSAGE 4

// global variables to store register values
static uint64_t saved_x0, saved_x1, saved_x2, saved_x8;

// returns: 
// - for syscalls: bits 63:32 = 0, bits 31:0 = syscall return value
// - for exceptions: bits 63:32 = 1, bits 31:0 = unused (halt)
uint64_t handle_sync_exception(uint64_t user_x0, uint64_t user_x1, uint64_t user_x2, uint64_t user_x8) {
    uint64_t esr, far, elr;
    asm volatile("mrs %0, esr_el1" : "=r"(esr));
    asm volatile("mrs %0, far_el1" : "=r"(far));
    asm volatile("mrs %0, elr_el1" : "=r"(elr));
    
    // extract exception class from ESR
    uint64_t ec = (esr >> 26) & 0x3F;
    
    // debug: Print the first exception only
    static int exception_count = 0;
    if (exception_count == 0) {
        // uart_puts("SYNC EXCEPTION: EC="); uart_hex(ec); uart_puts(" ESR="); uart_hex(esr); uart_puts(" FAR="); uart_hex(far); uart_puts(" ELR="); uart_hex(elr); uart_puts("\n");
        exception_count++;
    }
    
    // check if its syscall (SVC instruction from EL0)
    if (ec == 0x15) {  // SVC instruction
        // uart_puts("SYSCALL DETECTED!\n");
        
        // Use the passed register values instead of reading current registers
        saved_x0 = user_x0;
        saved_x1 = user_x1;
        saved_x2 = user_x2;
        saved_x8 = user_x8;
        
        // uart_puts("Syscall number: "); uart_hex(saved_x8); uart_puts("\n");
        // uart_puts("x0: "); uart_hex(saved_x0); uart_puts("\n");
        
        // handle syscall based on x8 (syscall number)
        switch(saved_x8) {
            case SYS_PUTC:
                {
                    struct Message msg;
                    msg.type = MSG_PUTC;
                    msg.character = (char)saved_x0;
                    msg.path = NULL;
                    msg.argv = NULL;
                    msg.data = NULL;
                    msg.size = 0;
                    msg.flags = 0;
                    msg.fd = 0;
                    msg.pid = 0;
                    msg.status = 0;
                    msg.entry = 0;
                    msg.dirents = NULL;
                    msg.dirent_count = 0;
                    msg.string = NULL;
                    send_message(&msg);
                    return 0;  
                }
                
            case SYS_GETC:
                {
                    struct Message msg;
                    msg.type = MSG_GETC;
                    msg.character = 0;
                    msg.path = NULL;
                    msg.argv = NULL;
                    msg.data = NULL;
                    msg.size = 0;
                    msg.flags = 0;
                    msg.fd = 0;
                    msg.pid = 0;
                    msg.status = 0;
                    msg.entry = 0;
                    msg.dirents = NULL;
                    msg.dirent_count = 0;
                    msg.string = NULL;
                    int result = send_message(&msg);
                    if (result >= 0) {
                        return (uint64_t)msg.character;  // return character as result
                    }
                    return (uint64_t)-1;  // error return value
                }
                
            case SYS_PUTS:
                {
                    struct Message msg;
                    msg.type = MSG_PUTS;
                    msg.character = 0;
                    msg.path = NULL;
                    msg.argv = NULL;
                    msg.data = NULL;
                    msg.size = 0;
                    msg.flags = 0;
                    msg.fd = 0;
                    msg.pid = 0;
                    msg.status = 0;
                    msg.entry = 0;
                    msg.dirents = NULL;
                    msg.dirent_count = 0;
                    msg.string = (char*)saved_x0;
                    send_message(&msg);
                    return 0;  
                }
                
            case SYS_SEND_MESSAGE:
                {
                    // x0 contains pointer to message structure
                    struct Message *user_msg = (struct Message*)saved_x0;
                    //uart_puts("DEBUG: SYS_SEND_MESSAGE called with ptr=");
                    //uart_hex((uint64_t)user_msg);
                    //uart_puts("\n");
                    
                    // try to access the message type
                    //uart_puts("DEBUG: Trying to read msg->type\n");
                    uint64_t msg_type = user_msg->type;
                    //uart_puts("DEBUG: msg->type = ");
                    //uart_hex(msg_type);
                    //uart_puts("\n");
                    
                    int result = send_message(user_msg);
                    //uart_puts("DEBUG: send_message returned ");
                    //uart_hex(result);
                    //uart_puts("\n");
                    return (uint64_t)result;  
                }
                
            default:
                uart_puts("Unknown syscall: ");
                uart_hex(saved_x8);
                uart_puts("\n");
                return (uint64_t)-1;  
        }
        
        // return from syscall by advancing ELR_EL1 past the SVC instruction
        asm volatile("mrs x1, elr_el1");
        asm volatile("add x1, x1, #4");
        asm volatile("msr elr_el1, x1");
    }
    
    // not a syscall so handle as error
    uart_puts("\n*** SYNC EXCEPTION CAUGHT! ***\n");
    uart_puts("ESR_EL1: "); uart_hex(esr); uart_puts("\n");
    uart_puts("EC: "); uart_hex(ec); uart_puts("\n");
    uart_puts("ELR_EL1: "); uart_hex(elr); uart_puts("\n");
    uart_puts("FAR_EL1: "); uart_hex(far); uart_puts("\n");
    
    // decode exception class (EC is bits 31:26 of ESR_EL1)
    switch(ec) {
        case 0x08:
            uart_puts("Exception: Instruction Abort from lower EL\n");
            break;
        case 0x09:
            uart_puts("Exception: Instruction Abort from same EL\n");
            break;
        case 0x0C:
            uart_puts("Exception: Data Abort from lower EL\n");
            break;
        case 0x0D:
            uart_puts("Exception: Data Abort from same EL\n");
            break;
        case 0x15:
            uart_puts("Exception: SVC instruction\n");
            break;
        case 0x20:  // Data Abort from lower EL (the one we're seeing)
            uart_puts("Exception: Data Abort from lower EL\n");
            break;
        case 0x21:  // Data Abort from same EL
            uart_puts("Exception: Data Abort from same EL\n");
            break;
        case 0x22:  // PC alignment fault
            uart_puts("Exception: PC alignment fault\n");
            break;
        default:
            uart_puts("Exception: Unknown class ");
            uart_hex(ec);
            uart_puts("\n");
            break;
    }
    
    uart_puts("System halted due to unhandled exception.\n");
    return 0x100000000ULL;  // Halt bits 63:32 = 1
}

#endif 