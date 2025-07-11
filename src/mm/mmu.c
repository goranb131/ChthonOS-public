#include <stdint.h>
#include "uart.h"

/* AttrIdx0 = normal WB/WA, AttrIdx1 = device-nGnRnE */
#define MAIR_VALUE  ((0xFFULL << 0) | (0x04ULL << 8))

#define AF           (1ULL << 10)
#define UXN          (1ULL << 54)
#define AP_RW_EL0    (1ULL << 6)          /* AP[1:0]=0b01 */
#define AP_RW_EL1    (0ULL << 6)          /* AP[1:0]=0b00 */
#define BLOCK        (0ULL << 1)
#define VALID        (1ULL << 0)
#define ATTRIDX(n)  ((uint64_t)(n) << 2)

#define L1_DESC(pa_gib, attridx, extra) \
        (((uint64_t)(pa_gib) << 30) | AF | ATTRIDX(attridx) | (extra) | \
         BLOCK | VALID)

/* one 4 KiB aligned level 1 table */
__attribute__((aligned(4096))) static uint64_t l1[512];

static inline void isb(void){ __asm__ volatile("isb"); }
static inline void write_mair(uint64_t v){ __asm__ volatile("msr mair_el1,%0"::"r"(v)); }
static inline void write_tcr (uint64_t v){ __asm__ volatile("msr tcr_el1,%0"::"r"(v)); }
static inline void write_ttbr0(void *p)  { __asm__ volatile("msr ttbr0_el1,%0"::"r"(p)); }
static inline void enable_mmu(void)
{
    __asm__ volatile(
        "mrs  x0, sctlr_el1\n"
        "orr  x0, x0, #1\n"            
        "bic  x0, x0, #(1<<28)\n"      
        "msr  sctlr_el1, x0");
    isb();
}

static inline uint64_t read_sctlr(void)
{
    uint64_t v;
    __asm__ volatile("mrs %0, sctlr_el1" : "=r"(v));
    return v;
}

static inline void write_sctlr(uint64_t v)
{
    __asm__ volatile("msr sctlr_el1,%0" :: "r"(v) : "memory");
    isb();
}

void mmu_init(void)
{
    uart_puts("MMU init start\n");

/* 0-1 GiB devices (UART) EL1 RW, UXN */
//l1[0] = L1_DESC(0, 1, UXN | AP_RW_EL1);
l1[0] = L1_DESC(0,      /* physical GiB */
                1,      /* AttrIdx1 = device */
                UXN | AP_RW_EL0);   // was AP_RW_EL1

/* 1-2 GiB kernel EL1 RW/X*/
l1[1] = L1_DESC(1, 0, AP_RW_EL1);

/* 2-3 GiB user code+stack  EL0 RW/X (no UXN bit = user can execute)*/
l1[2] = L1_DESC(2, 0, AP_RW_EL0);

uart_puts("MMU L1 Block 0: "); uart_hex(l1[0]); uart_puts("\n");
uart_puts("MMU L1 Block 1: "); uart_hex(l1[1]); uart_puts("\n");
uart_puts("MMU L1 Block 2: "); uart_hex(l1[2]); uart_puts("\n");


    uart_puts("L1 identity blocks set\n");

    write_mair(MAIR_VALUE);                 uart_puts("MAIR set\n");

    write_tcr(0x00000000808519ULL);         uart_puts("TCR set\n");

    write_ttbr0(l1);                        uart_puts("TTBR0 set\n");

    uart_puts("Enabling MMU...\n");
    enable_mmu();                           uart_puts("MMU enabled\n");

    
    /* temporary */
    //write_sctlr(read_sctlr() & ~1ULL);   // MMU off again
    //uart_puts("MMU really was on; turning it back OFF works\n");

    extern void exception_vector_table(void);
    __asm__ volatile("msr vbar_el1,%0"::"r"(&exception_vector_table));
    isb();
}

uint64_t mmu_get_l1_block(int i){ return l1[i]; }