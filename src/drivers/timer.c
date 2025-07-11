#include <stdint.h>  
#include "timer.h"
#include "uart.h"
#include "process.h"
#include "gic.h"


#define read_sysreg(reg) ({ \
    unsigned long _val; \
    asm volatile("mrs %0, " #reg : "=r"(_val)); \
    _val; \
})

#define write_sysreg(reg, val) ({ \
    unsigned long _val = (unsigned long)(val); \
    asm volatile("msr " #reg ", %0" :: "r"(_val)); \
})

#define cnthctl_el2 S3_4_C14_C1_0

#define TIMER_INTERVAL 62500000  

#define GICD_BASE       0x08000000UL
#define GICC_BASE       0x08010000UL
#define GICD_CTLR       ((volatile uint32_t*)(GICD_BASE + 0x0))
#define GICD_ISENABLER(n)   ((volatile uint32_t*)(GICD_BASE + 0x100 + 4 * (n)))
#define GICD_ITARGETSR(n)   ((volatile uint8_t*)(GICD_BASE + 0x800 + (n)))
#define GICD_IPRIORITYR(n)  ((volatile uint8_t*)(GICD_BASE + 0x400 + (n)))
#define GICC_CTLR          ((volatile uint32_t*)(GICC_BASE + 0x0))
#define GICC_PMR           ((volatile uint32_t*)(GICC_BASE + 0x4))
#define GICC_IAR           ((volatile uint32_t*)(GICC_BASE + 0xC))
#define GICC_EOIR          ((volatile uint32_t*)(GICC_BASE + 0x10))

void timer_init(void) {
    uart_puts("Timer initialized (disabled for user shell testing)\n");
    
    /*
    uint64_t cntfrq;
    asm volatile("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    
    uint64_t interval = cntfrq / 1000; 
    
    
    asm volatile("msr cntp_tval_el0, %0" :: "r" (interval));
    
    
    uint64_t cntp_ctl = 1; 
    asm volatile("msr cntp_ctl_el0, %0" :: "r" (cntp_ctl));
    
    
    gic_enable_interrupt(30);  
    
    
    asm volatile("msr daifclr, #2");
    */
}

void timer_handler(void) {
    
    unsigned int irq = gic_acknowledge_interrupt();
    
    
    uint64_t cntfrq;
    asm volatile("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    uint64_t interval = cntfrq / 1000;
    asm volatile("msr cntp_tval_el0, %0" :: "r" (interval));
    
    // schedule();
    
    gic_end_interrupt(irq);
}

void enable_timer_interrupt(void) {
    
    gic_enable_interrupt(30);  
} 