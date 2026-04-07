// https://pdos.csail.mit.edu/6.828/2010/readings/hardware/8259A.pdf
// https://wiki.osdev.org/8259_PIC
#include <stdint.h>
#include "pic.h"
#include "io.h"
#include "utils.h"
#include "term.h"

// Two types of "command words": initialization command word (ICW) and operation command word (OCW)
#define OCW2_EOI (0x20)

// LTIM ADI SNGL IC4
#define ICW1_IC4_PRESENT (0x01) // is ICW4 present
#define ICW1_SNGL_MODE (0x02) // single mode (only one PIC) or cascade mode (two PICs)
#define ICW1_CALL_INTERVAL (0x04) // address interval of 4 (1) or 8 (0) for different interrupt locations. note: this does nothing on x86
#define ICW1_LEVEL_INT_MODE (0x08) // level (1) or edge detect (0)
#define ICW1_INIT (0x10) // always set

#define ICW4_8086 (0x01) // 8086 mode (1) or 80/85 mode (0)

#define MASTER_PIC_CMD (0x20)
#define MASTER_PIC_DATA (0x21)
#define SLAVE_PIC_CMD (0xA0)
#define SLAVE_PIC_DATA (0xA1)

#define SLAVE_PIC_CASCADE_IRQ (2) // IRQ2 has slave PIC tied to it

// offset1 - master PIC offset
// offset2 - slave PIC offset

static uint8_t master_pic_offset;
static uint8_t slave_pic_offset;

void pic_init(uint8_t offset1){
  master_pic_offset = offset1;
  slave_pic_offset  = offset1 + 0x08;

  // ICW1: init + expect ICW4
  outb(MASTER_PIC_CMD, ICW1_INIT | ICW1_IC4_PRESENT); io_wait();
  outb(SLAVE_PIC_CMD, ICW1_INIT | ICW1_IC4_PRESENT); io_wait();

  // ICW2: vector offsets
  outb(MASTER_PIC_DATA, master_pic_offset); io_wait();
  outb(SLAVE_PIC_DATA, slave_pic_offset); io_wait();

  // ICW3: cascade
  outb(MASTER_PIC_DATA, 1 << SLAVE_PIC_CASCADE_IRQ); io_wait();
  outb(SLAVE_PIC_DATA, SLAVE_PIC_CASCADE_IRQ); io_wait();

  // ICW4: 8086 mode
  outb(MASTER_PIC_DATA, ICW4_8086); io_wait();
  outb(SLAVE_PIC_DATA, ICW4_8086); io_wait();

  pic_disable();
}

void pic_eoi(uint8_t vector){
  // Make sure within bounds
  ASSERT(vector >= master_pic_offset && vector < slave_pic_offset + 0x08);

  if(vector >= slave_pic_offset){
    outb(SLAVE_PIC_CMD, OCW2_EOI);
  }
  outb(MASTER_PIC_CMD, OCW2_EOI);
}

void pic_enable(){
  outb(MASTER_PIC_DATA, 0x00);
  io_wait();
  outb(SLAVE_PIC_DATA, 0x00);
  io_wait();
}

void pic_disable(){
  outb(MASTER_PIC_DATA, 0xFF);
  io_wait();
  outb(SLAVE_PIC_DATA, 0xFF);
  io_wait();
}

void pic_set_mask(uint8_t vector){
  ASSERT(vector >= master_pic_offset && vector < slave_pic_offset + 0x08);

  uint16_t port = MASTER_PIC_DATA;
  uint8_t value;
  vector -= master_pic_offset;

  if (vector >= 8){
    port = SLAVE_PIC_DATA;
    vector -= 8;
  }

  ASSERT(vector < 8);

  value = inb(port) | (1 << vector); io_wait();
  outb(port, value); io_wait();
}

void pic_clear_mask(uint8_t vector){
  ASSERT(vector >= master_pic_offset && vector < slave_pic_offset + 0x08);

  uint16_t port = MASTER_PIC_DATA;
  uint8_t value;
  vector -= master_pic_offset;

  if (vector >= 8){
    port = SLAVE_PIC_DATA;
    vector -= 8;
  }

  ASSERT(vector < 8);

  value = inb(port) & (~(1 << vector)); io_wait();
  outb(port, value); io_wait();
}

uint8_t pic_get_mask(uint8_t slave){
  // 0 gets master, 1 gets slave
  uint8_t res = inb((slave)?SLAVE_PIC_DATA:MASTER_PIC_DATA); io_wait();
  return res;
}
