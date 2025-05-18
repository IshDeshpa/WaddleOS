.code16

.section .text
.global _start
.type _start, @function

fb0: .string "Fail boot1"
sb0: .string "Success boot1"

_start:
  cli
  mov $0x0000, %ax
  mov %ax, %ss # stack segment
  mov $0x7C00, %sp # stack pointer

  mov $0x0100, %ax       # boot1 segment
  mov %ax, %cs
  mov %ax, %ds
  mov %ax, %es
  sti

lp: jmp lp
  
   
  
