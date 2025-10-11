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

// Tiny wait (1-4 microseconds)
#define io_wait() { \
  __asm__ volatile("outb %%al, $0x80" : : "a"(0)); \
  for(uint16_t i = 0; i < 0xFFFF; i++); \
}


#endif
