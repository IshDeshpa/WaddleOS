.global term_print
.global term_readcursor

.section .text._start
.global _start
.type _start, @function

.code16
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
  call _print16

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

  call term_readcursor

  mov $pm, %eax
  push %eax
  call term_print
  add $4, %esp

  jmp main

.code32
.section .text
.global load_sector_asm
.type load_sector_asm, @function

_save_cr0: 
  .long 0
_save_sp:
  .long 0

load_sector_asm:
  movzbl 2(%esp), %eax # sector number
  mov 4(%esp), %ebx # number of sectors (< 24)
  
  cmp $24, %ebx
  jg _ret

  mov %esp, _save_sp

  # 1) change dap
  lea dap, %edx
  movw %bx, 2(%edx) # move number of sectors to dap
  movw %ax, 8(%edx) # move sector number to dap

  # 2) jump to 16-bit protected mode
  cli 

  lgdt gdt_desc 

  ljmp $0x18, $0x1400

.code16
.section .pmode16, "ax"
.global _pmode16_load_sector
_pmode16_load_sector:
  cli
  mov $0x18, %ax
  mov %ax, %ds
  mov %ax, %es

  # disable paging just in case it's enabled. also disables 16-bit protected mode 
  mov %cr0, %eax
  mov %eax, [_save_cr0]
  and $0x7FFFFFFE, %eax
  mov %eax, %cr0

  ljmp $0x0000, $0x1500

.section .rmode, "ax"
.global _rmode_load_sector
_rmode_load_sector:
  # reload segment registers
  cli
  xor %ax, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %ss
  mov $0x7C00, %sp

  lidt ridtd

  sti

  # 3) load to disk buffer   
  # Setup INT 13h extension
  mov $0x4100, %ax # read sectors
  mov $0x55AA, %bx # offset
  mov $0x80, %dl # drive 0x80 (HDD)
  int $0x13
  
  # Error checking lol
  jc .

  lea dap, %si	 # disk address packet in DS:SI
  mov $0x42, %ah # read disk
  mov $0x80, %dl # drive 0x80 (HDD)
  int $0x13

  # Error checking lol
  jc .

  # 4) jump to protected mode
  cli
  lgdt gdt_desc
  mov %cr0, %eax
  or $1, %eax
  mov %eax, %cr0
  
  # similarly jank shit
  .byte 0xEA
  .word _clear_pipe2
  .word 0x0008

.code32
_clear_pipe2:
  mov $0x10, %ax       # Load data segment selector
  mov %ax, %ds         # Load data segment
  mov %ax, %es         # Load extra segment
  mov %ax, %fs         # Load FS segment
  mov %ax, %gs         # Load GS segment
  mov %ax, %ss         # Load stack segment
  mov _save_sp, %esp   # Set stack pointer
  mov _save_cr0, %eax
  mov %eax, %cr0


  # 5) load to appropriate address (done in C routine)
_ret:
  ret

.code16
.global _pchr16
.type _pchr16, @function
_pchr16:
  mov $0x0E, %ah
  mov $0x00, %bh
  int $0x10
  ret

.global _print16
.type _print16, @function
_print16:
  # Expects correct pointer in si
  # load ds:si into al and increment si
  lodsb 
  call _pchr16

  or %al, %al
  jz _prt16
  jmp _print16

_prt16:
  ret

.section .data
fb0: .ascii "Fail boot1\n\r\0"
sb0: .ascii "Success boot1\n\r\0"
a2: .ascii "A20 Gate Enable!\n\r\0"
pm: .ascii "Protected Mode Enable!\n\r\0"

.align 4
dap:
  .byte 0x10 	# size of DAP (16 bytes)
  .byte 0x00
  .word 24	# number of sectors to read (size of disk buffer)
  .word 0x0000
  .word 0x0800	# segment (0x0800, disk buffer) + offset (0x0000)
  .long 24	# sector to be read (starts at 24) 
  .long 0  

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
gdt_code16:
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
  .byte 0b00001111  // bit 19-16: top four bits of segment limit
            // bit 20: available for use by us (do whatever)
            // bit 21: reserved by intel
            // bit 22: 16-bit (0) or 32-bit (1) segment?
            // bit 23: 4kB multiplier if set
  .byte 0           // bits 31-24: top four bits of base address
gdt_end:
gdt_desc:
  .word gdt_end - gdt - 1// size of gdt (0-15)
  .long gdt       // gdt address (16-47)

.align 16

ridtd:
  .word 0x3FF
  .long 0x0
