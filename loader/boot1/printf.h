#ifndef PRINTF_H
#define PRINTF_H

typedef enum {
  DEV_TERM = 0x1,
  DEV_SERIAL_COM1 = 0x2
} printf_device_t;

int printf(printf_device_t device, const char *str, ...);

#endif
