#include <stdarg.h>
#include "term.h"

#define MAX_PRINTF_LEN (0xFFFFFFFF)

int printf(const char *str, ...){
  va_list ptr;
  va_start(ptr, str);
  
  for(int i=0; str[i] != '\0' && i < MAX_PRINTF_LEN; i++){
    if(str[i] == '%'){
      switch (str[i+1]) {
        case 's':
          term_print(va_arg(ptr, char*));
          break;
        case 'd':
          int a = va_arg(ptr, int);
          char buf[20];
          uint8_t i=0;

          while(a){
            buf[i++] = a%10;
            a/=10;
          }

          while(i > 0){
            term_putc(buf[--i] + '0');
          }

          break;
        case 'x':
          term_print("0x");
          term_putbyte(va_arg(ptr, int));
          break;
        default:
          term_putc('X');
          break;
      }

      i++;
    } else {
      term_putc(str[i]);
    }
  }

  va_end(ptr);
  return 0;
}
