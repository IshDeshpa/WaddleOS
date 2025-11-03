#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
  // General-purpose registers pushed by the stub
  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t r11;
  uint64_t r10;
  uint64_t r9;
  uint64_t r8;
  uint64_t rsi;
  uint64_t rdi;
  uint64_t rbp;
  uint64_t rdx;
  uint64_t rcx;
  uint64_t rbx;
  uint64_t rax;

  // CPU automatically pushes this
  uint64_t error_code; // dummy or real, depending on exception
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;  // only pushed if privilege level changes
  uint64_t ss;   // only pushed if privilege level changes
} interrupt_frame_t;

typedef enum {
  // CPU exceptions (0–31)
  EXC_DIVIDE_ERROR           = 0,
  EXC_DEBUG                  = 1,
  EXC_NMI_INTERRUPT          = 2,
  EXC_BREAKPOINT             = 3,
  EXC_OVERFLOW               = 4,
  EXC_BOUND_RANGE_EXCEEDED   = 5,
  EXC_INVALID_OPCODE         = 6,
  EXC_DEVICE_NOT_AVAILABLE   = 7,
  EXC_DOUBLE_FAULT           = 8,
  EXC_COPROCESSOR_SEG_OVERRUN= 9,
  EXC_INVALID_TSS            = 10,
  EXC_SEGMENT_NOT_PRESENT    = 11,
  EXC_STACK_SEGMENT_FAULT    = 12,
  EXC_GENERAL_PROTECTION     = 13,
  EXC_PAGE_FAULT             = 14,
  EXC_RESERVED_15            = 15,
  EXC_X87_FPU_ERROR          = 16,
  EXC_ALIGNMENT_CHECK        = 17,
  EXC_MACHINE_CHECK          = 18,
  EXC_SIMD_FP_EXCEPTION      = 19,
  EXC_VIRTUALIZATION         = 20,
  EXC_CONTROL_PROTECTION     = 21,
  EXC_RESERVED_22            = 22,
  EXC_RESERVED_23            = 23,
  EXC_RESERVED_24            = 24,
  EXC_RESERVED_25            = 25,
  EXC_RESERVED_26            = 26,
  EXC_RESERVED_27            = 27,
  EXC_RESERVED_28            = 28,
  EXC_RESERVED_29            = 29,
  EXC_RESERVED_30            = 30,
  EXC_RESERVED_31            = 31,

  // Hardware IRQs (32–47)
  ISR_PIT_TIMER              = 32,
  ISR_KEYBOARD               = 33,
  ISR_CASCADE                = 34,
  ISR_COM2                   = 35,
  ISR_COM1                   = 36,
  ISR_LPT2                   = 37,
  ISR_FLOPPY_DISK            = 38,
  ISR_LPT1                   = 39,
  ISR_RTC                    = 40,
  ISR_PERIPHERAL_1           = 41,
  ISR_PERIPHERAL_2           = 42,
  ISR_PERIPHERAL_3           = 43,
  ISR_MOUSE                  = 44,
  ISR_FPU                    = 45,
  ISR_PRIMARY_ATA            = 46,
  ISR_SECONDARY_ATA          = 47
} interrupt_vector_t;

void interrupts_init();
void interrupts_enable();
void interrupts_disable();
bool interrupts_active();
void interrupts_register_handler(uint8_t vector, void (*handler)(interrupt_frame_t *));

extern void exc_0();
extern void exc_1();
extern void exc_2();
extern void exc_3();
extern void exc_4();
extern void exc_5();
extern void exc_6();
extern void exc_7();
extern void exc_8();
extern void exc_9();
extern void exc_10();
extern void exc_11();
extern void exc_12();
extern void exc_13();
extern void exc_14();
extern void exc_15();
extern void exc_16();
extern void exc_17();
extern void exc_18();
extern void exc_19();
extern void exc_20();
extern void exc_21();
extern void exc_22();
extern void exc_23();
extern void exc_24();
extern void exc_25();
extern void exc_26();
extern void exc_27();
extern void exc_28();
extern void exc_29();
extern void exc_30();
extern void exc_31();

extern void isr_32();
extern void isr_33();
extern void isr_34();
extern void isr_35();
extern void isr_36();
extern void isr_37();
extern void isr_38();
extern void isr_39();
extern void isr_40();
extern void isr_41();
extern void isr_42();
extern void isr_43();
extern void isr_44();
extern void isr_45();
extern void isr_46();
extern void isr_47();

#endif
