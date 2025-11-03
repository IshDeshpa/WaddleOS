#ifndef PRINTF_H
#define PRINTF_H

#include <stdarg.h>

typedef enum {
  DEV_TERM = 0x1,
  DEV_SERIAL_COM1 = 0x2
} printf_device_t;

#define DEFAULT_DEVICE DEV_SERIAL_COM1

int vprintf_to_device(printf_device_t device, const char *str, va_list args);
int vprintf(const char *str, va_list args);
int printf_to_device(printf_device_t device, const char *str, ...);
int printf(const char *str, ...);

#endif
