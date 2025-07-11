    .section .text
    .global _start

#ifdef RPI4_BUILD
// RPI is 0x80000
.equ LOADADDR, 0x80000
#else
// QEMU virt machine default -kernel at 0x40000000
.equ LOADADDR, 0x40000000
#endif

_start:
    // exception vector table
    ldr x0, =exception_vector_table
    msr vbar_el1, x0

    // enable interrupts at EL1
    msr daifclr, #2

    // set stack pointer near the top of our region
    ldr x0, =stack_top
    mov sp, x0

    // branch to C
    bl kernel_main

hang:
    wfe
    b hang
