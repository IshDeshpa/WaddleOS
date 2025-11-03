// https://wiki.osdev.org/Serial_Ports#Example_Code

#include "serial.h"
#include "io.h"

int serial_init(uint16_t port) {
   outb(port + 1, 0x00);    // Disable all interrupts
   outb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
   outb(port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
   outb(port + 1, 0x00);    //                  (hi byte)
   outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
   outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
   outb(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
   outb(port + 4, 0x1E);    // Set in loopback mode, test the serial chip
   outb(port + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

   // Check if serial is faulty (i.e: not same byte as sent)
   if(inb(port + 0) != 0xAE) {
      return 1;
   }

   // If serial is not faulty set it in normal operation mode
   // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
   outb(port + 4, 0x0F);
   return 0;
}

static inline int serial_recvd(uint16_t port){
  return inb(port + 5) & 1;
}

char serial_read(uint16_t port){
  while(!serial_recvd(port));

  return inb(port);
}

static inline int serial_is_tx_empty(uint16_t port){
  return inb(port + 5) & 0x20;
}

void serial_write(uint16_t port, char a){
  while(serial_is_tx_empty(port) == 0);

  outb(port, a);
}

void serial_com1_putc(char a){
  serial_write(COM1, a);
}

void serial_com2_putc(char a){
  serial_write(COM2, a);
}
