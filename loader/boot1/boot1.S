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
  
  # 32-bit mode (index 2)
  ljmp $0x10, $_clear_pipe

.code32
_clear_pipe:
  mov $0x20, %ax       # Load data segment selector
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

.equ p4t_addr, 0x10000
.equ p3t_addr, 0x11000
.equ p2t_addr, 0x12000
.equ p1t_addr, 0x13000

.equ pt_addr_mask, 0xffffffffff000
.equ pt_present, 0x1
.equ pt_readable, 0x2
.equ pt_size, 4096

.equ pt_entries, 512
.equ pt_entry_size, 8
.equ pg_size, 4096

.equ efer_msr, 0xC0000080
.equ efer_lm, 0x100

.section .text
.code32
.global long_mode
.type long_mode, @function
long_mode:
  movl $p4t_addr, %edi
  movl %edi, %cr3 # tell the CPU where p4t is

  movl $0x0, %eax
  movl $pt_size, %ecx
  
  # repeat stosl (store string doubleword) ecx times
  # stores eax to addr edi and increments edi by 4
  # clears all four page tables
  rep stosl
  
  # reset eax to page table base
  mov %cr3, %eax
  
  # p4 first entry points to p3
  movl $((p3t_addr & pt_addr_mask) | pt_present | pt_readable), %eax
  movl $0, %edx
  movl %eax, p4t_addr
  movl %edx, p4t_addr+4

  # p3 first entry points to p2
  movl $((p2t_addr & pt_addr_mask) | pt_present | pt_readable), %eax
  movl $0, %edx
  movl %eax, p3t_addr
movl %edx, p3t_addr+4

  # p2 first entry points to p1
  movl $((p1t_addr & pt_addr_mask) | pt_present | pt_readable), %eax
  movl $0, %edx
  movl %eax, p2t_addr
  movl %edx, p2t_addr+4

  # set up PTE
  movl $p1t_addr, %eax
  movl $(pt_present | pt_readable), %ebx
  movl $pt_entries, %ecx

_set_entry: # loops 512 times
  mov %ebx, (%eax)
  movl $0, 4(%eax)
  add $pg_size, %ebx
  add $pt_entry_size, %eax
  # decrements ecx
  loop _set_entry 

  # enable PAE (Physical Address Extension)
  mov %cr4, %eax
  or $0x20, %eax
  mov %eax, %cr4

  # extended feature enable register has long mode bit
  mov $efer_msr, %ecx
  rdmsr
  or $efer_lm, %eax
  wrmsr

  # CR0 enable paging (last step to enable compatibility mode)
  mov %cr0, %eax
  # paging enable bit
  or $0x80000000, %eax
  mov %eax, %cr0

  ljmp $0x08, $_long_mode_entry

.code64
.section .data
.global _kernel_entry
.align 4
_kernel_entry:
  .quad 0

.section .text
_long_mode_entry:
  mov _kernel_entry(%rip), %rax   # load 64-bit pointer stored at _kernel_entry
  jmp *%rax                       # jump to kernel entry

.code32
.global check_for_longmode
.type check_for_longmode, @function
check_for_longmode:
  # 1) check for CPUID extended features
  mov $0x80000000, %eax
  cpuid
  cmp $0x80000001, %eax
  jb _no_long_mode

  # 2) check bit 29 of edx (long mode)
  mov $0x80000001, %eax
  cpuid
  bt $29, %edx
  jc _has_long_mode

_no_long_mode:
  movl $0x0, %eax
  ret

_has_long_mode:
  mov $0x1, %eax
  ret

.global check_for_cpuid
.type check_for_cpuid, @function
check_for_cpuid:
  # pushfl pushes 32-bit EFLAGS register to the stack
  pushfl
  # pop into EAX reg
  pop %eax
  # save eax to ecx and then try to flip bit 21
  mov %eax, %ecx
  xor $0x200000, %eax
  # push to stack
  pushl %eax
  # pop to EFLAGS
  popfl
  # read back
  pushfl
  pop %eax
  xor %ecx, %eax
  and $0x200000, %eax
  jz _no_cpuid

  mov $1, %eax
  ret

_no_cpuid:
  mov $0, %eax
  ret

.global load_sector_asm
.type load_sector_asm, @function
_save_cr0: 
  .long 0
_save_sp:
  .long 0

load_sector_asm:
  # Starts in 32-bit pmode

  movzwl 4(%esp), %eax # sector number
  movzwl 8(%esp), %ecx # number of sectors (< 24)
  
  # Make sure sector number >= 12
  cmp $12, %eax
  jnge _ret

  # Make sure number of sectors <= 24
  cmp $24, %ecx
  jnle _ret
  
  pusha
  mov %esp, _save_sp

  # 1) change dap
  lea dap, %edx
  movw %cx, 2(%edx) # move number of sectors to dap
  movw %ax, 8(%edx) # move sector number to dap

  # 2) jump to 16-bit protected mode
  cli 

  lgdt gdt_desc 
  
  # 16 bit mode (index 3) Pmode-16 address (defined in linker.ld)
  ljmp $0x18, $_pmode16_load_sector

.code16
.section .pmode16, "ax"
.global _pmode16_load_sector
_pmode16_load_sector:
  # 16-bit pmode
  cli
  mov $0x18, %ax
  mov %ax, %ds
  mov %ax, %es

  # disable paging just in case it's enabled. also disables 16-bit protected mode 
  mov %cr0, %eax
  mov %eax, [_save_cr0]
  and $0x7FFFFFFE, %eax
  mov %eax, %cr0

  # Real mode address, defined in linker.ld
  ljmp $0x0000, $_rmode_load_sector

.section .rmode, "ax"
.global _rmode_load_sector
_rmode_load_sector:
  # 16-bit rmode
  # reload segment registers
  cli
  xor %ax, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %ss
  mov $0x7C00, %sp

  lidt idt_desc

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
  .word 0x0010

.code32
_clear_pipe2:
  mov $0x20, %ax       # Load data segment selector
  mov %ax, %ds         # Load data segment
  mov %ax, %es         # Load extra segment
  mov %ax, %fs         # Load FS segment
  mov %ax, %gs         # Load GS segment
  mov %ax, %ss         # Load stack segment
  mov _save_sp, %esp   # Set stack pointer
  mov _save_cr0, %eax
  and $~0x80000000, %eax # no paging boooooo
  mov %eax, %cr0

  # 5) load to appropriate address (done in C routine)
  popa

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
gdt_code64:
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
gdt_code32:
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
            // bit 20: reserved
            // bit 21: long mode (0)
            // bit 22: 16-bit (0) or 32-bit (1) segment?
            // bit 23: 4kB multiplier if set
  .byte 0           // bits 31-24: top four bits of base address
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
            // bit 20: reserved
            // bit 21: long mode (0)
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

// think this is a dummy idt? but not sure
.align 8
idt_desc:
  .word 0x3FF
  .long 0x0
