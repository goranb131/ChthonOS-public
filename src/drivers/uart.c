#include "uart.h"

#ifdef RPI4_BUILD
#define UART_BASE 0xFE201000  
#else
#define UART_BASE 0x09000000  
#endif

#define UART_DR     (UART_BASE + 0x00)
#define UART_FR     (UART_BASE + 0x18)
#define UART_LCRH   (UART_BASE + 0x2C)
#define UART_CR     (UART_BASE + 0x30)
#define UART_IFLS   (UART_BASE + 0x34)
#define UART_IMSC   (UART_BASE + 0x38)

#define FR_TXFF     (1 << 5)  
#define FR_RXFE     (1 << 4)  
#define FR_BUSY     (1 << 3)  

static void mmio_write(unsigned long reg, unsigned int val) {
    *(volatile unsigned int *)reg = val;
}

static unsigned int mmio_read(unsigned long reg) {
    return *(volatile unsigned int *)reg;
}

void uart_init(void)
{
    
    mmio_write(UART_CR, 0);
    
    
    mmio_write(UART_IBRD, 26);     
    mmio_write(UART_FBRD, 3);      
    mmio_write(UART_LCRH, (1 << 4) | (1 << 5) | (1 << 6)); 
    
    
    mmio_write(UART_CR, (1 << 0) | (1 << 8) | (1 << 9));  
}

void uart_putc(char c) {
    
    while (mmio_read(UART_FR) & FR_TXFF) { }
    mmio_write(UART_DR, c);
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
        
        if ((s - (const char *)0) % 8 == 0) {
            while (mmio_read(UART_FR) & FR_BUSY) { }
        }
    }
    
    while (mmio_read(UART_FR) & FR_BUSY) { }
}


void uart_hex(unsigned long n) {
    uart_puts("0x");
    for(int i = 60; i >= 0; i -= 4) {
        int digit = (n >> i) & 0xF;
        uart_putc(digit + (digit < 10 ? '0' : 'a' - 10));
    }
}

char uart_getc(void) {
    while (mmio_read(UART_FR) & FR_RXFE) { }  
    return mmio_read(UART_DR) & 0xFF;  
}
