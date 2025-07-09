/* uart_debug.c */
#include "uart.h"

void uart_dump_regs(unsigned long spsr,
                    unsigned long elr,
                    unsigned long sp)
{
    uart_puts("SPSR_EL1="); uart_hex(spsr); uart_puts("  ");
    uart_puts("ELR_EL1=" ); uart_hex(elr ); uart_puts("  ");
    uart_puts("SP_EL0="  ); uart_hex(sp ); uart_puts("\n");
}