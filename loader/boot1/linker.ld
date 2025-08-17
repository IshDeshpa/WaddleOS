ENTRY(_start)

SECTIONS
{
  . = 0x1000;

  .text :
  {
    KEEP(*(.text._start))
    *(.text)
  }

  .rodata :
  {
    *(.rodata*)
  }

  .data :
  {
    *(.data)
  }

  bss :
  {
    *(.bss COMMON)
  }
  
  .pmode16 :
  {
    *(.pmode16)
  }

  .rmode :
  {
    *(.rmode)
  }
}

