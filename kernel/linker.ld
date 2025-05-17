ENTRY(_start)

SECTIONS{

  /* Load at 1Mb */
  . = 1M;
  
  /* Multiboot header */
  .text : 
  {
     KEEP(*(.multiboot))
     *(.text)
  }

  /* Read only data */
  .rodata : ALIGN(4K)
  {
    *(.rodata)
  }

  /* Read-write data initialized */
  .data : ALIGN(4K)
  {
    *(.data)
  }

  /* Read-write data uninitialized; stack */
  .bss : ALIGN(4K)
  {
    *(COMMON)
    *(.bss)
  }
}
