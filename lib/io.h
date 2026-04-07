#ifndef IO_H
#define IO_H

#include <stdint.h>

/* Read a byte from an I/O port */
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0"
                      : "=a"(val)
                      : "Nd"(port));
    return val;
}

/* Write a byte to an I/O port */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1"
                      :
                      : "a"(val), "Nd"(port));
}

/* Tiny I/O wait (optional) */
static inline void io_wait(void) {
    __asm__ volatile("outb %%al, $0x80" : : "a"(0));
}

#endif
