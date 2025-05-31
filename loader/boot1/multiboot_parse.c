#include "multiboot2.h"
#include <stdint.h>
#include "term.h"
void load_sector(uint16_t sector, uint16_t num_sectors, uint8_t* address);

int main(){
  load_sector(12, 1, (uint8_t*)0x100000);
  term_print("Load sector finished!\n\r");
  while(1){}
}
