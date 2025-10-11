#ifndef PIC_H
#define PIC_H

#include <stdint.h>

void pic_init(uint8_t offset1);
void pic_eoi(uint8_t irq);
void pic_disable();
void pic_enable();
void pic_set_mask(uint8_t vector);
void pic_clear_mask(uint8_t vector);
uint8_t pic_get_mask(uint8_t slave);

#endif
