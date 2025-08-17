/**
 * Parse multiboot2 header for kernel being loaded
 * 
 * https://www.cs.cmu.edu/~410-s22/p4/p4-boot.pdf
 * https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html
 *
 */

#include "multiboot2.h"
#include <stdint.h>
#include "string.h"
#include "term.h"

// Num_sectors must be <= 24 and sector must be >= 12
void load_sector_asm(uint16_t sector, uint16_t num_sectors);
void load_sector(uint16_t sector, uint16_t num_sectors, uint8_t* address){
  load_sector_asm(sector, num_sectors); // loads num sectors starting at sector into 0x8000
  const void *disk_buf = (const void *)0x8000;

  // Memcopy
  memcpy(address, disk_buf, ((size_t)num_sectors << 9));
  
  // Print out first 16 bytes
  for(uint8_t *i=(uint8_t*)disk_buf; i<((uint8_t*)disk_buf + 16); i++){
    term_putbyte(*(i) & 0xFF);
    term_putc('\n');
  }
}

#define KERNEL_START_ADDR ((uint8_t*)0x100000)
#define KERNEL_START_SECTOR (12)
#ifndef KERNEL_NUM_SECTORS
  #define KERNEL_NUM_SECTORS (9)
#endif

uint8_t* search_for_header(uint8_t* start){
  for(uint16_t i=0; i<0x8000; i+=4){
    if(*(uint32_t*)(start + i) == MULTIBOOT2_HEADER_MAGIC){
      term_print("Multiboot2 header spotted ");
      term_print("\n at ");
      term_print("0x");
      term_putbyte((((uint32_t)start+i) >> 8) & 0xFF);
      term_putbyte(((uint32_t)start+i) & 0xFF);
      term_putc('\n');
      return (start + i);
    }
  }
  return (uint8_t*)((uint32_t)-1);
}

int main(){
  load_sector(KERNEL_START_SECTOR, KERNEL_NUM_SECTORS, KERNEL_START_ADDR);
  term_print("Load sector finished!\n");
  uint8_t* header_loc = search_for_header(KERNEL_START_ADDR);
  while(1){}
}

