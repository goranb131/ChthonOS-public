OUTPUT_ARCH(aarch64)

#ifdef RPI4_BUILD
  /* build for RPI at 0x80000 */
  . = 0x80000;
#else
  /* Qemu at 0x40000000 */
  . = 0x40000000;
#endif

ENTRY(_start)

SECTIONS
{
  /* kernel code and read‐only data */
  .text : {
    *(.text*)
  }

  .rodata : {
    *(.rodata*)
  }

  /* initialized and uninitialized data */
  .data : {
    *(.data*)
  }

  .bss : {
    _bss_start = .;
    *(.bss*)
    *(COMMON)
    _bss_end = .;
  }

  . = ALIGN(16);

  /*  user_stack_bottom = low address
      user_stack_top    = high address */
  user_stack_bottom = .;
  . += 0x4000;           
  user_stack_top    = .;

  . = ALIGN(16);

  /* 
       stack_top = high address 
  */
  stack_top = . + 0x4000;

   /* ───────────────────────────────────────────────
     EL0 test code to 0x8000 0000 (1 GiB above
     the kernel identity-mapped region)
     ─────────────────────────────────────────────── */
  .user 0x80000000 : { *(.text.user) }
  .user 0x80000000 : { *(.user) }
}