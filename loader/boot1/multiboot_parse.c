#include "multiboot2.h"
#include <stdint.h>
#include "term.h"

void load_sector_asm(uint16_t sector, uint16_t num_sectors);
void load_sector(uint16_t sector, uint16_t num_sectors, uint8_t* address){
  load_sector_asm(sector, num_sectors); // loads num sectors starting at sector into 0x8000
}

int main(){
  load_sector(24, 1, (uint8_t*)0x100000);
  term_print("Load sector finished!\n");
  while(1){}
}
