// http://www.osdever.net/bkerndev/Docs/pit.htm
#include "interrupts.h"
#include "utils.h"
#include "io.h"
#include "printf.h"

void timer_interrupt(interrupt_frame_t *interrupt_frame){
  term_printf("TIMER INTERRUPT\n\r");
}

void pit_init(){
  ASSERT(interrupts_active());
  interrupts_register_handler(0x20, timer_interrupt);
}


