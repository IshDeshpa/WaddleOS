#include "idt.h"
#include "paging.h"
#include "term.h"
#include "serial.h"
#include "printf.h"
#include "interrupts.h"
#include "pic.h"
#include "pit.h"
#include "utils.h"
#include "multiboot2.h"

#ifdef __linux__
  #error "ERROR: Must be compiled via cross-compiler"
#elif !defined(__x86_64__)
  #error "ERROR: Must be compiled with an x86-elf compiler"
#endif

extern uint32_t mboot_magic;

void kernel_main(){
  interrupts_disable(); // -------
  
  ASSERT(mboot_magic == MULTIBOOT2_BOOTLOADER_MAGIC);

  term_clear();
  serial_init(COM1);
  
  interrupts_init();

  // Memory init
  paging_init();

  pit_init(100); // 100 hz
  
  interrupts_enable(); // -------

  for(int i=0; i<100; i++){
    printf("Hello World! %s %d %x\n\r", "abc", i, i);

    for(int j=0; j<50000000; j++); // delay
  }
}
