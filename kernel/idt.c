#include "idt.h"
#include "term.h"
#include <stdint.h>

#define IDT_MAX_DESCRIPTORS (256)

__attribute__((aligned(0x10))) static idt_entry_t idt[IDT_MAX_DESCRIPTORS]; // Create an array of IDT entries; aligned for performance
static idtr_t idtr;

static void *isr_stubs[32] = {
  &isr_0, &isr_1, &isr_2, &isr_3,
  &isr_4, &isr_5, &isr_6, &isr_7,
  &isr_8, &isr_9, &isr_10, &isr_11,
  &isr_12, &isr_13, &isr_14, &isr_15,
  &isr_16, &isr_17, &isr_18, &isr_19,
  &isr_20, &isr_21, &isr_22, &isr_23,
  &isr_24, &isr_25, &isr_26, &isr_27,
  &isr_28, &isr_29, &isr_30, &isr_31,
};

void idt_init(){
  idtr.base = (uint64_t) &idt[0];
  idtr.limit = sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;

  uint8_t flags = 0x8E; // interrupt gate, present, ring 0

  for(uint8_t vector=0; vector < 32; vector++){
    idt_set_descriptor(vector, isr_stubs[vector], flags);
  }

  __asm__ volatile("lidt %0" : : "m"(idtr));
}

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags){
  idt_entry_t *descriptor = &idt[vector];

  descriptor->isr_low = (uint64_t)isr & 0xFFFF;
  descriptor->isr_mid = ((uint64_t)isr >> 16) & 0xFFFF;
  descriptor->isr_high = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
  descriptor->ist = 0;
  descriptor->kernel_cs = 0x08; // my 64 bit selector
  descriptor->attributes = flags;
  descriptor->reserved = 0;
}

void enable_interrupts(){
  __asm__ volatile("cli" ::: "memory");
}

void disable_interrupts(){
  __asm__ volatile("sti" ::: "memory");
}

void exception_handler(interrupt_frame_t *interrupt_frame) {
  (void) interrupt_frame;
  term_print("exception_handler");
  while(1) {
    __asm__ volatile ("hlt");
  }
}
