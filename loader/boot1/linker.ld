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

  . = 0x1400;
  .pmode16 :
  {
    *(.pmode16)
  }

  . = 0x1500;
  .rmode :
  {
    *(.rmode)
  }
}

