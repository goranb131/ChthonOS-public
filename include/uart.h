#ifndef UART_H
#define UART_H

#define UART_IBRD   (UART_BASE + 0x24)
#define UART_FBRD   (UART_BASE + 0x28)

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_hex(unsigned long n);
char uart_getc(void);

#endif
