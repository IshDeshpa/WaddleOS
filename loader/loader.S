.code16

.section .text
.global _start
.type _start, @function

_start:
  cli
  ljmp $0x07C0, $start

fb0: .string "Fail boot0"
sb0: .string "Success boot0"
_disp:
  # Expect carry bit to contain success or fail
  # This should move carry bit into %ax
  xor %ax, %ax        # Clear AX
  adc %ax, %ax        # Add CF to AX → AX = 0 or 1

  mov $0x0E, %ah
  mov $fb0, %si

_p_lp:
  # load from ds:si into al, and increment si
  lodsb
  or %al, %al
  jz _p_done
  int $0x10
  jmp _p_lp
  
_p_done:
  or %ax, %ax
  jz _p_fin
lp: 
  jmp lp
_p_fin:
  ret

start:
  # Base of boot1 is 0x1000 in %es segment
  mov $0x07C0, %ax # will be multiplied by 10 and added to whatever offset is used
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs
  mov %ax, %ss
  mov $0x07C0, %sp

  sti

  # Reset disk to known state
  mov $0x00, %ah
  int $0x13
  
  # Read from disk
  mov $0x02, %ah
  mov $0x11, %al # total sector count = 17 sectors (0x1000 to 0x31ff)
  mov $0x00, %ch # cylinder is 0
  mov $0x02, %cl # boot1 starts at sector 2
  mov $0x00, %dh # head is 0
  movw $0x1000, %bx # offset for segment of 0x1000 is 0
  int $0x13  
  
  call _disp

  # else, jump to 0x1000 to start boot1
  ljmp $0x07C0, $0x1000
 
.org 510              # Pad to 510 bytes from _start
.byte 0x55, 0xAA              # Boot signature
