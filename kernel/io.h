#ifndef IO_H
#define IO_H

#include <stdint.h>

#define inb(port) ({ \
    uint8_t _ret; \
    __asm__ volatile ("inb %1, %0" \
                      : "=a"(_ret) \
                      : "Nd"((uint16_t)(port))); \
    _ret; \
})
#define outb(val, port) __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)val), "Nd"((uint16_t)port))

#endif
