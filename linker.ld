ENTRY(start)

SECTIONS{

  // Load at 1Mb
  . = 1M;
  
  // Multiboot header
  .rodata BLOCK(4K) : ALIGN(4K)
  {
    *(.multiboot)
  }

  // Code
  .text BLOCK(4K) : ALIGN(4K)
  {
    *(.text)
  }
  
  // Read only data
  .rodata BLOCK(4K) : ALIGN(4K)
  {
    *(.rodata)
  }

  // Read-write data initialized
  .data BLOCK(4K) : ALIGN(4K)
  {
    *(.data)
  }

  // Read-write data uninitialized; stack
  .bss BLOCK(4K) : ALIGN(4K)
  {
    *(COMMON)
    *(.bss)
  }
}
