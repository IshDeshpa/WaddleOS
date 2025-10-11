#include "interrupts.h"

#include <stdint.h>
#include "idt.h"
#include "pic.h"
#include "utils.h"
#include "printf.h"

static void *exc_stubs[32] = {
  &exc_0, &exc_1, &exc_2, &exc_3,
  &exc_4, &exc_5, &exc_6, &exc_7,
  &exc_8, &exc_9, &exc_10, &exc_11,
  &exc_12, &exc_13, &exc_14, &exc_15,
  &exc_16, &exc_17, &exc_18, &exc_19,
  &exc_20, &exc_21, &exc_22, &exc_23,
  &exc_24, &exc_25, &exc_26, &exc_27,
  &exc_28, &exc_29, &exc_30, &exc_31,
};

static void (*exc_handlers[32])(interrupt_frame_t *interrupt_frame) = {0};

static void *isr_stubs[16] = {
  &isr_32, &isr_33, &isr_34, &isr_35,
  &isr_36, &isr_37, &isr_38, &isr_39,
  &isr_40, &isr_41, &isr_42, &isr_43,
  &isr_44, &isr_45, &isr_46, &isr_47,
};

static void (*isr_handlers[16])(interrupt_frame_t *interrupt_frame) = {0};

#define INT_FLAGS_PRESENT (0x80)
#define INT_FLAGS_DPL_KERNEL (0x00)
#define INT_FLAGS_DPL_USER (0x60)
#define INT_FLAGS_GATE_INTR (0b1110)
#define INT_FLAGS_GATE_TRAP (0b1111)

void interrupts_init(){
  // Initialize exceptions (internal)
  uint8_t flags = INT_FLAGS_PRESENT | INT_FLAGS_DPL_KERNEL | INT_FLAGS_GATE_TRAP;
  for(uint8_t vector=0; vector < 32; vector++){
    idt_set_descriptor(vector, exc_stubs[vector], flags);
  }

  // Initialize interrupts (external hardware)
  flags = INT_FLAGS_PRESENT | INT_FLAGS_DPL_KERNEL | INT_FLAGS_GATE_INTR;
  pic_init(0x20);
  for(uint8_t irq_num=32; irq_num<48; irq_num++){
    idt_set_descriptor(irq_num, isr_stubs[irq_num], flags);
  }

  // Initialize IDT
  idt_init();
}

void interrupts_register_handler(uint8_t vector, void (*handler)(interrupt_frame_t *)){
  if(vector < 32) exc_handlers[vector] = handler;
  else if(vector < 48){
    isr_handlers[vector - 32] = handler;
    pic_clear_mask(vector);
  }
}

static bool i_active = false;
void interrupts_enable(){
  __asm__ volatile("cli" ::: "memory");
  i_active = true;
}

void interrupts_disable(){
  __asm__ volatile("sti" ::: "memory");
  i_active = false;
}

bool interrupts_active(){
  return i_active;
}

void exception_handler(interrupt_frame_t *interrupt_frame, uint64_t vector_number) {
  term_printf("Vector Number %d\n\r", vector_number);
 
  ASSERT(vector_number < 32);
  ASSERT(exc_handlers[vector_number] != 0);

  exc_handlers[vector_number](interrupt_frame);
}

void interrupt_handler(interrupt_frame_t *interrupt_frame, uint64_t irq_number){
  term_printf("IRQ %d\n\r", irq_number);

  ASSERT(irq_number >= 32 && irq_number < 48);
  ASSERT(isr_handlers[irq_number - 32] != 0);

  isr_handlers[irq_number - 32](interrupt_frame);

  pic_eoi(irq_number);
}
