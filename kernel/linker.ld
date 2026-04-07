ENTRY(_start)

VIRT_BASE = 0xffffffff80100000;
PHYS_BASE = 0x100000;
LMA_OFS = PHYS_BASE - VIRT_BASE;

SECTIONS{

  /* Load at 1Mb */
  . = VIRT_BASE;
  
  /* Multiboot header */
  .text : AT(ADDR(.text) + LMA_OFS) 
  {
     KEEP(*(.multiboot))
     *(.text*)
  } 

  /* Read only data */
  .rodata : AT(ADDR(.rodata) + LMA_OFS)
  {
    *(.rodata*)
  }

  /* Read-write data initialized */
  .data : AT(ADDR(.data) + LMA_OFS)
  {
    *(.data*)
  } 

  /* Read-write data uninitialized; stack */
  .bss : AT(ADDR(.bss) + LMA_OFS)
  {
    _bss_start = .;
    *(COMMON)
    *(.bss*)
    _bss_end = .;

    . = ALIGN(16);
    _stack_bottom = .;
    . += 16K;
    _stack_top = .;
  } 

  _kernel_end = .;
}
