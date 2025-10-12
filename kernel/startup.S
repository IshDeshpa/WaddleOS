#define ASM_FILE 1
#include "multiboot2.h"

.section .multiboot
multiboot_hdr:
  .long MULTIBOOT2_HEADER_MAGIC
  .long MULTIBOOT_ARCHITECTURE_I386
  .long multiboot_hdr_end - multiboot_hdr
  .long -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + multiboot_hdr_end - multiboot_hdr)
multiboot_tag_align:
  .align MULTIBOOT_TAG_ALIGN
  .word MULTIBOOT_HEADER_TAG_MODULE_ALIGN
  .word 0
  .long 8
multiboot_tag_console_flags:
  .align MULTIBOOT_TAG_ALIGN
  .word MULTIBOOT_HEADER_TAG_CONSOLE_FLAGS
  .word 0
  .long 12
  .long 3
multiboot_tag_framebuffer:
  .align MULTIBOOT_TAG_ALIGN
  .word MULTIBOOT_HEADER_TAG_FRAMEBUFFER
  .word 0
  .long 20
  .long 0
  .long 0
  .long 32
multiboot_hdr_end:
  .align MULTIBOOT_TAG_ALIGN

.extern _bss_start
.extern _bss_end
.extern _stack_top
.extern _stack_bottom

.section .data
.align 4
gdt:
gdt_null:
  .quad 0 // quad = quad word (64-bit)
gdt_code:
  .word 0xFFFF // ignored by 64-bit mode
  .word 0 // ignored by 64-bit mode
  .byte 0 // ignored by 64-bit mode
  .byte 0b10011000 // bit 8: unused
                  // bit 9: unused
                  // bit 10: conforming (can less privileged segments call this segment?) (0)
                  // bit 11: unused
                  // bit 12: unused
                  // bit 14-13: privilege level (00 most privileged)
                  // bit 15: present? (1)
  .byte 0b00100000 // bit 19-16: unused
                   // bit 20: unused
                   // bit 21: long mode flag (1)
                   // bit 22: 16-bit (0) or 32-bit (1) segment? (should be 0 if long mode selected)
                   // bit 23: unused
  .byte 0          // bits 31-24: unused
gdt_data:
  .word 0x0000
  .word 0x0000
  .byte 0x00
  .byte 0b10010010    # Data: read/write, ring 0, present
  .byte 0x00
  .byte 0x00
gdt_end:
gdt_desc:
  .word gdt_end - gdt - 1 // size of gdt (0-15)
  .quad gdt      // gdt address (16-47)

.section .text
.global _start
.type _start, @function

.code64
_start:
  cli // disable interrupts

  // Stack pointer
  lea _stack_top(%rip), %rsp
  xor %rbp, %rbp // clear base pointer

  // Load GDT
  lgdt [gdt_desc]

  mov $0x10, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs
  mov %ax, %ss

  // Initialize bss
  lea _bss_start(%rip), %rdi
  lea _bss_end(%rip), %rcx
  sub %rdi, %rcx
  xor %rax, %rax
  rep stosb

  // Nifty trick; GAS for some reason doesn't like me ljmping to next, so using this form of memory-based ljmp
  // where I push the segment and the address of next to the stack and then "long returning"
  pushq $0x08 
  lea next(%rip), %rax
  pushq %rax 
  lretq

next:
  // Call main
  call kernel_main
  
  // Infinite loop
  cli
lp: 
  hlt
  jmp lp

.size _start, . - _start
