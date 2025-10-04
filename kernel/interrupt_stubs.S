/* 
 * GAS 64-bit interrupt/exception stub macro
 *
 * Arguments:
 * 1 = vector number (used in label)
 * 2 = 0 if no error code is pushed by CPU, 1 if CPU pushes error code
 */

.macro ISR_STUB vector, has_error_code
.global isr_\vector
.align 16
isr_\vector:
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

/* CPU exceptions ISR stubs (vectors 0-31) */

/* Exceptions without error code */
ISR_STUB 0, 0   /* Divide Error (#DE) */
ISR_STUB 1, 0   /* Debug (#DB) */
ISR_STUB 2, 0   /* Non-maskable Interrupt (#NMI) */
ISR_STUB 3, 0   /* Breakpoint (#BP) */
ISR_STUB 4, 0   /* Overflow (#OF) */
ISR_STUB 5, 0   /* Bound Range Exceeded (#BR) */
ISR_STUB 6, 0   /* Invalid Opcode (#UD) */
ISR_STUB 7, 0   /* Device Not Available (#NM) */
ISR_STUB 9, 0   /* Coprocessor Segment Overrun (#C) */
ISR_STUB 15,0   /* Reserved */
ISR_STUB 16,0   /* x87 Floating-Point Exception (#MF) */
ISR_STUB 18,0   /* Machine Check (#MC) */
ISR_STUB 19,0   /* SIMD Floating-Point Exception (#XF) */
ISR_STUB 20,0   /* Virtualization Exception (#VE) */
ISR_STUB 21,0   /* Reserved */
ISR_STUB 22,0   /* Reserved */
ISR_STUB 23,0   /* Reserved */
ISR_STUB 24,0   /* Reserved */
ISR_STUB 25,0   /* Reserved */
ISR_STUB 26,0   /* Reserved */
ISR_STUB 27,0   /* Reserved */
ISR_STUB 28,0   /* Reserved */
ISR_STUB 29,0   /* Reserved */
ISR_STUB 30,0   /* Security Exception (#SX) */
ISR_STUB 31,0   /* Reserved */

/* Exceptions with error code */
ISR_STUB 8, 1   /* Double Fault (#DF) */
ISR_STUB 10,1   /* Invalid TSS (#TS) */
ISR_STUB 11,1   /* Segment Not Present (#NP) */
ISR_STUB 12,1   /* Stack-Segment Fault (#SS) */
ISR_STUB 13,1   /* General Protection (#GP) */
ISR_STUB 14,1   /* Page Fault (#PF) */
ISR_STUB 17,1   /* Alignment Check (#AC) */
