#ifndef _EXCEPTIONS_H
#define _EXCEPTIONS_H

unsigned long handle_sync_exception(unsigned long user_x0, unsigned long user_x1, unsigned long user_x2, unsigned long user_x8);
void handle_irq(void);

#endif 