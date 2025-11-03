#define ASM_FILE 1
#include "multiboot2.h"
#include "constants.h"

.section .text._start
.global _start
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

  jmp main

.global load_sector_asm
.type load_sector_asm, @function
.code32
load_sector_asm:
  pushl %ebp
  movl %esp, %ebp

  pushal                  # saves eax, ecx, edx, ebx, esp, ebp, esi, edi
  pushfl                  # save flags

  # get arguments
  mov 8(%ebp), %ax # sector number
  mov 12(%ebp), %bx # number of sectors

  # Make sure sector number >= 12
  cmp $12, %ax
  jnge _ret_load_sector_asm

  # Make sure number of sectors <= 24
  cmp $24, %bx
  jnle _ret_load_sector_asm

  # 1) change dap
  lea dap, %edx
  movw %bx, 2(%edx) # move number of sectors to dap
  movw %ax, 8(%edx) # move sector number to dap
 
  pushw $_rmode_load_sector # push function pointer as subroutine to run in real mode
  call run_rmode_subrt
 
  # pop arguments so that stack restores properly
  popw %ax

_ret_load_sector_asm:
  popfl
  popal
  leave
  ret

.global _rmode_load_sector
.type _rmode_load_sector, @function
.code16
_rmode_load_sector:
  push %bp
  mov %sp, %bp

  # 3) load to disk buffer   
  # Setup INT 13h extension
  movw $0x4100, %ax # read sectors
  movw $0x55AA, %bx # offset
  movb $0x80, %dl # drive 0x80 (HDD)
  int $0x13
 
  # Error checking lol
  jc .

  lea dap, %si	 # disk address packet in DS:SI
  movb $0x42, %ah # read disk
  movb $0x80, %dl # drive 0x80 (HDD)
  int $0x13

  # Error checking lol
  jc .

  leave
  ret

.section .data
mem_lower_addr: .long 0
mem_upper_addr: .long 0

.section .text
.global get_basic_meminfo
.type get_basic_meminfo, @function
.code32
get_basic_meminfo:
  push %ebp
  movl %esp, %ebp
  
  pushal
  pushfl

  pushw $_rmode_get_basic_meminfo 
  call run_rmode_subrt
  popw %ax

  # save in passed addresses
  movl 8(%ebp), %esi
  movl (mem_lower_addr), %eax
  movl %eax, (%esi)

  movl 12(%ebp), %esi
  movl (mem_upper_addr), %eax
  movl %eax, (%esi)

_ret_get_basic_meminfo:
  popfl
  popal
  leave
  ret

.global _rmode_get_basic_meminfo
.type _rmode_get_basic_meminfo, @function
.code16
_rmode_get_basic_meminfo:
  push %bp
  mov %sp, %bp

  xor %ax, %ax
  xor %bx, %bx

  # get lower and upper mem size
  mov $0xE801, %ax
  int $0x15
  jc .

  # store lower mem
  mov $mem_lower_addr, %si
  mov %ax, (%si)
  
  # multiply bx by 64
  shl $6, %bx
  # store in mem upper
  mov $mem_upper_addr, %si
  mov %bx, (%si)

  leave
  ret

.section .data
single_mmap_entry:
  .skip 24
ebx_ret:
  .long 0
cl_ret:
  .byte 0
mmap_finished:
  .byte 0

.section .text
.global get_single_mmap
.type get_single_mmap, @function
.code32
get_single_mmap:
  push %ebp
  movl %esp, %ebp
  
  pushal
  pushfl

  mov $0x0, (mmap_finished)

  pushw $_rmode_get_single_mmap
  call run_rmode_subrt
  popw %ax

  # move to address passed by user
  mov 8(%ebp), %edi
  mov $single_mmap_entry, %esi
  mov $24, %ecx
  cld
  rep movsb

_ret_get_single_mmap:
  popfl
  popal

  movzbl mmap_finished, %eax

  leave
  ret

.global _rmode_get_single_mmap
.type _rmode_get_single_mmap, @function
.code16
_rmode_get_single_mmap:
  push %bp
  mov %sp, %bp

  mov $0xE820, %ax
  mov (ebx_ret), %bx
  mov $24, %cx
  movl $0x534D4150, %edx
  movl $single_mmap_entry, %edi
  int $0x15

  # Failure
  cmp $0x534D4150, %eax
  jne .

  cmp $0x0, %ebx
  jne _not_done
  mov $1, (mmap_finished)

_not_done:
  movl %ebx, (ebx_ret)
  movb %cl, (cl_ret)

  leave
  ret

.global run_rmode_subrt
.type run_rmode_subrt, @function
.code32
run_rmode_subrt:
  pushl %ebp
  movl %esp, %ebp

  pushal
  pushfl

  mov $0x18, %ax
  mov %ax, %ds
  mov %ax, %es

  # Real mode address, defined in linker.ld
  ljmp $0x18, $_pmode16_entry


.code16
_pmode16_entry:
  # disable paging just in case it's enabled. also disables 16-bit protected mode 
  mov %cr0, %eax
  and $0x7FFFFFFE, %eax
  mov %eax, %cr0

  ljmp $0x0000, $_real_mode_entry

.code16
_real_mode_entry: 
  cli
  xor %ax, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %gs
  mov %ax, %fs
  mov %ax, %ss

  mov $idt_desc, %si
  lidt (%si)
  sti

  movw 8(%bp), %bx
  call *%bx

  mov   %cr0, %eax
  or    $1, %eax          # set PE bit again
  mov   %eax, %cr0
  ljmp  $0x10, $pmode_return  # jump back to 32-bit protected mode

.code32
pmode_return:
  cli

  mov $0x20, %ax       # Load data segment selector
  mov %ax, %ds         # Load data segment
  mov %ax, %es         # Load extra segment
  mov %ax, %fs         # Load FS segment
  mov %ax, %gs         # Load GS segment
  mov %ax, %ss         # Load stack segment

  popfl
  popal

  leave
  ret

.code32
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

.equ p4t_addr, 0x10000
.equ p3t_addr, 0x11000
.equ p3t_2_addr, 0x12000
.equ p2t_addr, 0x13000
.equ p1t_addr, 0x14000

.equ pt_addr_mask, 0xffffffffff000
.equ pt_present, 0x1
.equ pt_readable, 0x2
.equ pt_size, 4096

.equ pt_entries, 512
.equ pt_entry_size, 8
.equ pg_size, 4096

.equ efer_msr, 0xC0000080
.equ efer_lm, 0x100

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
  
  # for the identity map
  movl $((p3t_addr & pt_addr_mask) | pt_present | pt_readable), %eax
  movl %eax, p4t_addr
  movl $0, %edx
  movl %edx, p4t_addr+4

  # for the high level map
  movl $((p3t_2_addr & pt_addr_mask) | pt_present | pt_readable), %eax
  movl %eax, p4t_addr + (511 * pt_entry_size)
  movl $0, %edx
  movl %edx, p4t_addr + (511 * pt_entry_size) + 4

  # p3 first entry points to p2
  movl $((p2t_addr & pt_addr_mask) | pt_present | pt_readable), %eax
  movl $0, %edx
  movl %eax, p3t_addr
  movl %edx, p3t_addr+4

  # p3_2 510th entry points to p2
  movl $((p2t_addr & pt_addr_mask) | pt_present | pt_readable), %eax
  movl $0, %edx
  movl %eax, p3t_2_addr + (510 * pt_entry_size)
  movl %edx, p3t_addr + (510 * pt_entry_size) + 4

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

  # Return info to kernel
  movl $MULTIBOOT2_BOOTLOADER_MAGIC, %eax
  movl $MULTIBOOT_RETURN_INFO_ADDR, %ebx

  ljmp $0x08, $_long_mode_entry

.code64
.section .data
.global _kernel_entry
.align 4
_kernel_entry:
  .quad 0

.section .text
_long_mode_entry:
  mov _kernel_entry(%rip), %rsi   # load 64-bit pointer stored at _kernel_entry
  jmp *%rsi                       # jump to kernel entry

.section .data
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
