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

  .bss :
  {
    *(.bss COMMON)
  }
}

