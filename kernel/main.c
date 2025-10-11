#include "term.h"
#include "interrupts.h"
#include "pit.h"
#include "printf.h"

#ifdef __linux__
  #error "ERROR: Must be compiled via cross-compiler"
#elif !defined(__x86_64__)
  #error "ERROR: Must be compiled with an x86-elf compiler"
#endif

void kernel_main(){
  interrupts_disable(); // -------

  term_clear();
  
  interrupts_init();

  pit_init();

  interrupts_enable(); // -------

  char strbuf[2];
  strbuf[1] = '\0';
  for(int i=0; i<100; i++){
    term_printf("Hello World! %s %d %x\n\r", "abc", i, i);


    for(int j=0; j<50000000; j++); // delay
  }
}
