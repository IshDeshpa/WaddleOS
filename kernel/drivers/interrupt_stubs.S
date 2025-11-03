.macro ISR_STUB irq_num
.global isr_\irq_num
.align 16
isr_\irq_num:
  /* Save general-purpose registers */
  pushq %rax
  pushq %rbx
  pushq %rcx
  pushq %rdx
  pushq %rbp
  pushq %rsi
  pushq %rdi
  pushq %r8
  pushq %r9
  pushq %r10
  pushq %r11
  pushq %r12
  pushq %r13
  pushq %r14
  pushq %r15

  /* Pass pointer to stack frame in RDI */
  movq %rsp, %rdi

  /* Pass vector number in RSI */
  movq $\irq_num, %rsi

  /* Call common C handler */
  call interrupt_handler

  /* Restore general-purpose registers */
  popq %r15
  popq %r14
  popq %r13
  popq %r12
  popq %r11
  popq %r10
  popq %r9
  popq %r8
  popq %rdi
  popq %rsi
  popq %rbp
  popq %rdx
  popq %rcx
  popq %rbx
  popq %rax

  /* Return from interrupt */
  iretq
.endm

.macro EXC_STUB vector, has_error_code
.global exc_\vector
.align 16
exc_\vector:
  /* Save general-purpose registers */
  pushq %rax
  pushq %rbx
  pushq %rcx
  pushq %rdx
  pushq %rbp
  pushq %rsi
  pushq %rdi
  pushq %r8
  pushq %r9
  pushq %r10
  pushq %r11
  pushq %r12
  pushq %r13
  pushq %r14
  pushq %r15

  /* If no error code pushed by CPU, push dummy 0 */
  .if \has_error_code == 0
      pushq $0
  .endif

  /* Pass pointer to stack frame in RDI */
  movq %rsp, %rdi

  /* Pass vector number in RSI */
  movq $\vector, %rsi

  /* Call common C handler */
  call exception_handler

  /* Clean up dummy error code if we pushed one */
  .if \has_error_code == 0
      addq $8, %rsp
  .endif

  /* Restore general-purpose registers */
  popq %r15
  popq %r14
  popq %r13
  popq %r12
  popq %r11
  popq %r10
  popq %r9
  popq %r8
  popq %rdi
  popq %rsi
  popq %rbp
  popq %rdx
  popq %rcx
  popq %rbx
  popq %rax

  /* Return from interrupt */
  iretq
.endm

/* Exceptions without error code */
EXC_STUB 0, 0   /* Divide Error (#DE) */
EXC_STUB 1, 0   /* Debug (#DB) */
EXC_STUB 2, 0   /* Non-maskable Interrupt (#NMI) */
EXC_STUB 3, 0   /* Breakpoint (#BP) */
EXC_STUB 4, 0   /* Overflow (#OF) */
EXC_STUB 5, 0   /* Bound Range Exceeded (#BR) */
EXC_STUB 6, 0   /* Invalid Opcode (#UD) */
EXC_STUB 7, 0   /* Device Not Available (#NM) */
EXC_STUB 9, 0   /* Coprocessor Segment Overrun (#C) */
EXC_STUB 15,0   /* Reserved */
EXC_STUB 16,0   /* x87 Floating-Point Exception (#MF) */
EXC_STUB 18,0   /* Machine Check (#MC) */
EXC_STUB 19,0   /* SIMD Floating-Point Exception (#XF) */
EXC_STUB 20,0   /* Virtualization Exception (#VE) */
EXC_STUB 21,0   /* Reserved */
EXC_STUB 22,0   /* Reserved */
EXC_STUB 23,0   /* Reserved */
EXC_STUB 24,0   /* Reserved */
EXC_STUB 25,0   /* Reserved */
EXC_STUB 26,0   /* Reserved */
EXC_STUB 27,0   /* Reserved */
EXC_STUB 28,0   /* Reserved */
EXC_STUB 29,0   /* Reserved */
EXC_STUB 30,0   /* Security Exception (#SX) */
EXC_STUB 31,0   /* Reserved */ 

/* Exceptions with error code */
EXC_STUB 8, 1   /* Double Fault (#DF) */
EXC_STUB 10,1   /* Invalid TSS (#TS) */
EXC_STUB 11,1   /* Segment Not Present (#NP) */
EXC_STUB 12,1   /* Stack-Segment Fault (#SS) */
EXC_STUB 13,1   /* General Protection (#GP) */
EXC_STUB 14,1   /* Page Fault (#PF) */
EXC_STUB 17,1   /* Alignment Check (#AC) */ 

/* Interrupts 0x20 - 0x2F */ 
ISR_STUB 32
ISR_STUB 33
ISR_STUB 34
ISR_STUB 35
ISR_STUB 36
ISR_STUB 37
ISR_STUB 38
ISR_STUB 39
ISR_STUB 40
ISR_STUB 41
ISR_STUB 42
ISR_STUB 43
ISR_STUB 44
ISR_STUB 45
ISR_STUB 46
ISR_STUB 47
