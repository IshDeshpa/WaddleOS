// http://www.osdever.net/bkerndev/Docs/pit.htm
// https://wiki.osdev.org/Programmable_Interval_Timer
#include "interrupts.h"
#include "utils.h"
#include "printf.h"
#include "io.h"

#define LOG_LEVEL 1
#define LOG_ENABLE 0
#include "log.h"

void timer_interrupt(interrupt_frame_t *interrupt_frame){
  log(LOG_TRACE, "TIMER INTERRUPT\n\r");
}

/**
  * Bits         Usage
  *  7 and 6      Select channel :
  *                  0 0 = Channel 0
  *                  0 1 = Channel 1
  *                  1 0 = Channel 2
  *                  1 1 = Read-back command (8254 only)
  *  5 and 4      Access mode :
  *                  0 0 = Latch count value command (used to freeze counter)
  *                  0 1 = Access mode: lobyte only
  *                  1 0 = Access mode: hibyte only
  *                  1 1 = Access mode: lobyte/hibyte
  *  3 to 1       Operating mode :
  *                  0 0 0 = Mode 0 (interrupt on terminal count)
  *                  0 0 1 = Mode 1 (hardware re-triggerable one-shot)
  *                  0 1 0 = Mode 2 (rate generator)
  *                  0 1 1 = Mode 3 (square wave generator)
  *                  1 0 0 = Mode 4 (software triggered strobe)
  *                  1 0 1 = Mode 5 (hardware triggered strobe)
  *                  1 1 0 = Mode 2 (rate generator, same as 010b)
  *                  1 1 1 = Mode 3 (square wave generator, same as 011b)
  *  0            BCD/Binary mode: 0 = 16-bit binary, 1 = four-digit BCD
  */

#define PIT_DATA_0 (0x40)
#define PIT_DATA_1 (0x41)
#define PIT_DATA_2 (0x42)
#define PIT_COMMAND (0x43)

#define PIT_CHAN_SEL_0 (0x00)
#define PIT_CHAN_SEL_1 (0x40)
#define PIT_CHAN_SEL_2 (0x80)
#define PIT_CHAN_SEL_RB (0xC0)

#define PIT_ACCESSMODE_LATCH (0x00)
#define PIT_ACCESSMODE_LOBYTE (0x10)
#define PIT_ACCESSMODE_HIBYTE (0x20)
#define PIT_ACCESSMODE_LOHIBYTE (0x30)

#define PIT_OPERATING_MODE_0 (0x00)
#define PIT_OPERATING_MODE_1 (0x02)
#define PIT_OPERATING_MODE_2 (0x04)
#define PIT_OPERATING_MODE_3 (0x06)
#define PIT_OPERATING_MODE_4 (0x08)
#define PIT_OPERATING_MODE_5 (0x0A)

// BCD is binary coded decimal
#define PIT_BCD_OFF (0x00)
#define PIT_BCD_ON (0x01)

#define PIT_INITIAL_FREQ (1193180) // Hz

void pit_init(uint16_t timer_freq_hz){
  ASSERT(!interrupts_active());

  interrupts_register_handler(0x20, timer_interrupt);

  // Timer channels at 0x40 (chan 0), 0x41 (chan 1), 0x42 (chan 2)
  // Command register at 0x43 
  uint16_t divisor = PIT_INITIAL_FREQ / timer_freq_hz;
  outb(PIT_COMMAND, (PIT_CHAN_SEL_0 | PIT_ACCESSMODE_LOHIBYTE | PIT_OPERATING_MODE_3));

  outb(PIT_DATA_0, divisor & 0xFF);
  outb(PIT_DATA_0, (divisor >> 8) & 0xFF);
}


