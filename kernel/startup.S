.set ALIGN, 1<<0
.set MEMINFO, 1<<1
.set FLAGS, ALIGN | MEMINFO
.set MAGIC, 0xE85250D6
.set CHECKSUM, -(MAGIC+FLAGS)

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:

.section .data
.align 4
gdt:
gdt_null:
  .quad 0 // quad = quad word (64-bit)
gdt_code:
  .word 0xFFFF // hex FFFF; 4GB limit for this code segment
  .word 0 // base address is 0
  .byte 0 // bits 16-23 for base address
  .byte 0b10011010  // bit 8: access flag (0)
		// bit 9: readable? (1)
		// bit 10: conforming (can less privileged segments call this segment?) (0)
		// bit 11: execute? (1)
		// bit 12: system (0) or code/data (1)? (1)
		// bit 14-13: privilege level (00 most privileged)
		// bit 15: present? (1)
  .byte 0b11001111  // bit 19-16: top four bits of segment limit
		// bit 20: available for use by us (do whatever)
		// bit 21: reserved by intel
		// bit 22: 16-bit (0) or 32-bit (1) segment?
		// bit 23: 4kB multiplier if set
  .byte 0		// bits 31-24: top four bits of base address
gdt_data:
  .word 0xFFFF
  .word 0
  .word 0
  .byte 0b10010010
  .byte 0b11001111
  .byte 0
gdt_end:

gdt_desc:
  .word gdt_end - gdt // size of gdt (0-15)
  .long gdt	      // gdt address (16-47)

.section .text
.global _start
.type _start, @function

.code32
_start:
  cli // disable interrupts
  
  // set up gdt
  xorw %ax, %ax // clear ax
  movw %ds, %ax // set ds to 0 (base address)
  lgdt [gdt_desc]
  
  // enter protected mode
  movl %eax, %cr0 // eax = cr0
  orb %al, 1 // eax[0] = 1
  movl %cr0, %eax // cr0 = eax
  
  // clear pipeline by jumping and reload segment registers
  ljmp $0x01, $clear_pipe

clear_pipe:
  movw %ax, 0x01
  movw %ds, %ax
  movw %es, %ax
  movw %fs, %ax
  movw %gs, %ax
  movw %ss, %ax

  // Setup stack space
  mov $stack_top, %esp
  
  // Call main
  call kernel_main
  
  // Infinite loop
    cli
lp: hlt
    jmp lp

.size _start, . - _start
