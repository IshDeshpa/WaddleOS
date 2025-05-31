.code16
.section .text
.global _start
.type _start, @function
_start:
  cli
  ljmp $0x0000, $go

go:
  # Base of boot0 is 0x7C00
  mov $0x0000, %ax # will be multiplied by 10 and added to whatever offset is used
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs
  mov %ax, %ss
  mov $0x7C00, %sp
  
  sti

  # Reset disk to known state
  mov $0x00, %ah
  int $0x13

  # Setup INT 13h extension
  mov $0x41, %ah # read sectors
  mov $0x55AA, %bx # offset
  int $0x13
  
  # Error checking lol
  jc .

  lea dap, %si	 # disk address packet in DS:SI
  mov $0x42, %ah # read disk
  int $0x13

  # Error checking lol
  jc .

  # Expect carry bit to contain success or fail
  # This should move carry bit into %ax
  xor %ax, %ax        # Clear AX
  adc %ah, %ah        # Add CF to AX → AX = 0 or 1
  
  call _print

  # else, jump to 0x1000 to start boot1
  ljmp $0x0000, $0x1000

fb0: .ascii "Fail boot0\n\r\0"
sb0: .ascii "Success boot0\n\r\0"

.align 4
dap:
  .byte 0x10 # size of DAP (16 bytes)
  .byte 0x00
  .word 3	   # number of sectors to read (increase if needed)
  .word 0x0000
  .word 0x0100	# segment (0x0100) + offset (0x0000)
  .long 5	   # sector to be read (5)
  .long 0

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
  lea fb0, %si
  push %ax
  # Carry bit in %ah
  or %ah, %ah
  jnz _plp
  lea sb0, %si

_plp:
  # load ds:si into al and increment si
  lodsb
  
  call _pchr

  or %al, %al
  jz _pdone
  jmp _plp

_pdone:
  pop %ax
  or %ah, %ah
  jz _rt
  
  cli
  hlt
  jmp .

_rt:
  ret

.org 510              # Pad to 510 bytes from _start
.byte 0x55, 0xAA              # Boot signature
