#include "gic.h"


#define GICD_BASE 0x08000000
#define GICC_BASE 0x08010000

#define GICD_CTLR        0x000
#define GICD_ISENABLER   0x100
#define GICD_ICENABLER   0x180

#define GICC_CTLR        0x000
#define GICC_PMR         0x004
#define GICC_IAR         0x00C
#define GICC_EOIR        0x010

static void mmio_write(unsigned long addr, unsigned int val) {
    *(volatile unsigned int *)addr = val;
}

static unsigned int mmio_read(unsigned long addr) {
    return *(volatile unsigned int *)addr;
}

void gic_init(void) {
    /*
    mmio_write(GICD_BASE + GICD_CTLR, 1);
    
    mmio_write(GICC_BASE + GICC_CTLR, 1);
    
    mmio_write(GICC_BASE + GICC_PMR, 0xFF);
    
    gic_enable_interrupt(30);
    */
}

void gic_enable_interrupt(int irq) {
    unsigned int reg_offset = GICD_ISENABLER + ((irq / 32) * 4);
    unsigned int bit = 1U << (irq % 32);
    mmio_write(GICD_BASE + reg_offset, bit);
}

unsigned int gic_acknowledge_interrupt(void) {
    return mmio_read(GICC_BASE + GICC_IAR);
}

void gic_end_interrupt(unsigned int irq) {
    mmio_write(GICC_BASE + GICC_EOIR, irq);
} 