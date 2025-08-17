#ifndef TERM_H
#define TERM_H
#include <stdint.h>

void term_clear();

void term_putc(char c);

void term_print(const char* str);

void term_putbyte(uint8_t b);

#endif
