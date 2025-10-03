#include "term.h"
#include "interrupts.h"

#ifdef __linux__
  #error "ERROR: Must be compiled via cross-compiler"
#elif !defined(__x86_64__)
  #error "ERROR: Must be compiled with an x86-elf compiler"
#endif

void kernel_main(){
  term_clear();

  /*
  char strbuf[2];
  strbuf[1] = '\0';
  for(int i=0; i<100; i++){
    term_print("Hello, World! ");
    strbuf[0] = (i&0xF) + '0';
    term_print(strbuf);
    term_print("\n");
    for(int j=0; j<50000000; j++); // delay
  }
  */
  
   
  configure_interrupts();
}
