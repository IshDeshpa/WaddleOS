// https://pdos.csail.mit.edu/6.828/2010/readings/hardware/8259A.pdf
// https://wiki.osdev.org/8259_PIC
#include <stdint.h>
#include "pic.h"
#include "io.h"
#include "utils.h"

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
  slave_pic_offset = offset1 + 0x08;

  // Remaps IRQs 0-7,8-15 to offset1 and offset2 respectively
  outb(MASTER_PIC_CMD, ICW1_INIT | ICW1_IC4_PRESENT);
  outb(SLAVE_PIC_CMD, ICW1_INIT | ICW1_IC4_PRESENT);
  outb(MASTER_PIC_DATA, master_pic_offset); // master PIC vector offset (ICW2)
  outb(SLAVE_PIC_DATA, slave_pic_offset); // slave PIC vector offset (ICW2)
  outb(MASTER_PIC_DATA, (1 << SLAVE_PIC_CASCADE_IRQ)); // ICW3 (master)
  outb(SLAVE_PIC_DATA, 2); // ICW3 (slave)
  
  // ICW4: specify 8086 mode
  outb(MASTER_PIC_DATA, ICW4_8086);
  outb(SLAVE_PIC_DATA, ICW4_8086);

  pic_disable();

  // unmask
  // pic_enable();
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
  outb(MASTER_PIC_CMD, 0x00);
  outb(SLAVE_PIC_CMD, 0x00);
}

void pic_disable(){
  outb(MASTER_PIC_CMD, 0xFF);
  outb(SLAVE_PIC_CMD, 0xFF);
}

void pic_set_mask(uint8_t vector){
  ASSERT(vector >= master_pic_offset && vector < slave_pic_offset + 0x08);

  uint16_t port = MASTER_PIC_DATA;
  uint8_t value;
  vector -= master_pic_offset;

  if (vector > 8){
    port = SLAVE_PIC_DATA;
    vector -= 8;
  }

  value = inb(port) | (1 << vector);
  outb(value, port);
}

void pic_clear_mask(uint8_t vector){
  ASSERT(vector >= master_pic_offset && vector < slave_pic_offset + 0x08);

  uint16_t port = MASTER_PIC_DATA;
  uint8_t value;
  vector -= master_pic_offset;

  if (vector > 8){
    port = SLAVE_PIC_DATA;
    vector -= 8;
  }

  value = inb(port) & (~(1 << vector));
  outb(value, port);
}
