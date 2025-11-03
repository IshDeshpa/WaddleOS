#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

#include "printf.h"
#include "serial.h"
#include "term.h"

static inline void (*get_device_putc(printf_device_t device))(char){
    switch (device) {
        case DEV_TERM:   return term_putc;
        case DEV_SERIAL_COM1: return serial_com1_putc;
        default:         return NULL;
    }
}

static inline void print(void (*putc)(char), char *str){
  for(size_t i=0; str[i]!='\0'; i++) putc(str[i]);
}

#define MAX_PRINTF_LEN (0xFFFFFFFF)
int printf(printf_device_t device, const char *str, ...){
  void (*putc)(char) = get_device_putc(device);

  va_list ptr;
  va_start(ptr, str);
  
  for(unsigned int i=0; str[i] != '\0' && i < MAX_PRINTF_LEN; i++){
    if(str[i] == '%'){
      switch (str[i+1]) {
        case 's': {
          print(putc, va_arg(ptr, char*));
          break;
        }
        case 'd': {
          int a = va_arg(ptr, int);
          char buf[20];
          uint8_t i = 0;

          if (a == 0) {
              putc('0');
              break;
          }

          if (a < 0) {
              putc('-');
              a = -a;
          }

          while (a > 0 && i < sizeof(buf)) {
              buf[i++] = a % 10;
              a /= 10;
          }

          while (i > 0) {
              putc('0' + buf[--i]);
          }
          break;
        }
        case 'x': {
          uint64_t val = va_arg(ptr, uint64_t);
          char hex[17]; // 16 digits + null terminator
          for (int k = 0; k < 16; k++) {
              int nibble = (val >> ((15 - k) * 4)) & 0xF;
              if (nibble < 10) hex[k] = '0' + nibble;
              else hex[k] = 'a' + (nibble - 10);
          }
          hex[16] = '\0';
          print(putc, "0x");
          print(putc, hex);
          break;
        }
        default: {
          putc('X');
          break;
        }
      }

      i++;
    } else {
      putc(str[i]);
    }
  }

  va_end(ptr);
  return 0;
}
