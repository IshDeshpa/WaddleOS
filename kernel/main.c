#include "term.h"
#include "idt.h"

#ifdef __linux__
  #error "ERROR: Must be compiled via cross-compiler"
#elif !defined(__x86_64__)
  #error "ERROR: Must be compiled with an x86-elf compiler"
#endif

void test_divide_by_zero() {
    int a = 1;
    int b = 0;
    int c = a / b;  // should invoke #DE exception
    (void)c;
}

void kernel_main(){
  term_clear();
  
  idt_init();
  enable_interrupts();

  test_divide_by_zero();

  char strbuf[2];
  strbuf[1] = '\0';
  for(int i=0; i<100; i++){
    term_print("Hello, World! ");
    strbuf[0] = (i&0xF) + '0';
    term_print(strbuf);
    term_print("\n");
    for(int j=0; j<50000000; j++); // delay
  }
}
