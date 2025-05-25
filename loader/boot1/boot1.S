.code16

.section .text
.global _start
.type _start, @function

_start:
  cli
 
  mov $0x0000, %ax
  mov %ax, %ds
  mov %ax, %ss
  mov $0x6C00, %sp

  # A20 Gate

  # busy wait for input buffer
_lp_inp:
  inb $0x64, %al  # grab from input buffer of keyboard (0x64)
  testb $0x02, %al # test bit 1 (inp. buffer full bit)
  jnz _lp_inp	# if set, keep looping

  mov $0xD1, %al	# "write to output port" command
  outb %al, $0x64	# send command to port 64

  # busy wait for input buffer
_lp_inp2:
  inb $0x64, %al
  testb $0x02, %al
  jnz _lp_inp2

  mov $0xDF, %al	# "set a20 bit only" command
  outb %al, $0x60

  # A20 now enabled!
  mov $a2, %si
  call _print

  cli
  lgdt gdt_desc
  mov %cr0, %eax
  or $1, %eax
  mov %eax, %cr0
  
  ljmp $0x08, $_clear_pipe

.code32
_clear_pipe:
  mov $0x10, %ax       # Load data segment selector
  mov %ax, %ds         # Load data segment
  mov %ax, %es         # Load extra segment
  mov %ax, %fs         # Load FS segment
  mov %ax, %gs         # Load GS segment
  mov %ax, %ss         # Load stack segment
  mov $0x9FC00, %esp   # Set stack pointer
  
  jmp .

.code16
.global _pchr
.type _pchr, @function
_pchr:
  mov $0x0E, %ah
  mov $0x00, %bh
  int $0x10
  ret

.global _print
.type _print, @function
_print:
  # Expects correct pointer in si
  # load ds:si into al and increment si
  lodsb 
  call _pchr

  or %al, %al
  jz _rt
  jmp _print

_rt:
  ret
  
fb0: .string "Fail boot1"
sb0: .string "Success boot1"
a2: .ascii "A20 Gate Enable!\n\r\0"
pm: .ascii "Protected Mode Enable!\n\r\0"

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
  .byte 0           // bits 31-24: top four bits of base address
gdt_data:
  .word 0xFFFF
  .word 0
  .byte 0
  .byte 0b10010010
  .byte 0b11001111
  .byte 0
gdt_end:
gdt_desc:
  .word gdt_end - gdt - 1// size of gdt (0-15)
  .long gdt       // gdt address (16-47)

