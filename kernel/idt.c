#include "idt.h"
#include "printf.h"
#include <stdint.h>

#define IDT_MAX_DESCRIPTORS (256)

__attribute__((aligned(0x10))) static idt_entry_t idt[IDT_MAX_DESCRIPTORS]; // Create an array of IDT entries; aligned for performance
static idtr_t idtr;

void idt_init(){
  idtr.base = (uint64_t) &idt[0];
  idtr.limit = sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;

  __asm__ volatile("lidt %0" : : "m"(idtr));
}

void idt_set_descriptor(uint8_t vector, void *service_routine, uint8_t flags){
  idt_entry_t *descriptor = &idt[vector];

  descriptor->isr_low = (uint64_t)service_routine & 0xFFFF;
  descriptor->isr_mid = ((uint64_t)service_routine >> 16) & 0xFFFF;
  descriptor->isr_high = ((uint64_t)service_routine >> 32) & 0xFFFFFFFF;
  descriptor->ist = 0;
  descriptor->kernel_cs = 0x08; // my 64 bit selector
  descriptor->attributes = flags;
  descriptor->reserved = 0;
}

