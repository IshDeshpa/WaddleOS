#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

#define COM1 0x3F8
#define COM2 0x2F8

int serial_init(uint16_t port);
char serial_read(uint16_t port);
void serial_write(uint16_t port, char a);
void serial_com1_putc(char a);
void serial_com2_putc(char a);

#endif
