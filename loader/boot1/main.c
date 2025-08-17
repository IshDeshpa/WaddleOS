/**
 * https://www.cs.cmu.edu/~410-s22/p4/p4-boot.pdf
 * https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html
 */

#include "multiboot2.h"
#include <stdint.h>
#include "string.h"
#include "term.h"

#include "elf.h"

#define KERNEL_START_ADDR ((uint8_t*)0x100000)
#define KERNEL_START_SECTOR (12)
#ifndef KERNEL_NUM_SECTORS
  #define KERNEL_NUM_SECTORS (9)
#endif
#define DISK_BUFFER_ADDR ((uint8_t*)0x8000)

// Num_sectors must be <= 24 and sector must be >= 12
void load_sector_asm(uint16_t sector, uint16_t num_sectors);
void load_sector(uint16_t sector, uint16_t num_sectors, uint8_t* address){
  load_sector_asm(sector, num_sectors); // loads num sectors starting at sector into 0x8000
  // Memcopy
  memcpy(address, ((const void*)DISK_BUFFER_ADDR), ((size_t)num_sectors << 9));
  
  // Print out first 16 bytes
  for(uint8_t *i=(uint8_t*)DISK_BUFFER_ADDR; i<((uint8_t*)DISK_BUFFER_ADDR + 16); i++){
    term_putbyte(*(i) & 0xFF);
    term_putc('\n');
  }
}

/*
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
}*/
static Elf64_Ehdr local_ehdr;

int main(){
  //load_sector(KERNEL_START_SECTOR, KERNEL_NUM_SECTORS, KERNEL_START_ADDR);
  //term_print("Load sector finished!\n");
  //uint8_t* header_loc = search_for_header(KERNEL_START_ADDR);
  //

  // Load a sector into disk buffer
  load_sector_asm(KERNEL_START_SECTOR, 1);

  // Cast to ELF64 header (0x40 bytes long)
  Elf64_Ehdr *e_header = (Elf64_Ehdr *)DISK_BUFFER_ADDR;
  memcpy(&local_ehdr, e_header, sizeof(Elf64_Ehdr));
    
  // Confirm ELF magic number
  if(memcmp(e_header->e_ident, ELFMAG, SELFMAG) != 0){
    term_print("ELF magic incorrect!\n");
    while(1);
  }

  term_print("Elf header obtained!\n");

  // Load program header
  for(uint16_t i=local_ehdr.e_phoff; i<local_ehdr.e_phoff + local_ehdr.e_phnum * local_ehdr.e_phentsize; i+=local_ehdr.e_phentsize){
    load_sector_asm(KERNEL_START_SECTOR + (i>>9), (local_ehdr.e_phentsize>>9)+2);
    Elf64_Phdr *phdr = (Elf64_Phdr *)(DISK_BUFFER_ADDR + ((local_ehdr.e_phoff + i*local_ehdr.e_phentsize)%0x200));

    term_print("Load\n 0x");
    term_putbyte(phdr->p_filesz >> 8);
    term_putbyte(phdr->p_filesz);
    term_print(" bytes to \n 0x");
    term_putbyte(phdr->p_vaddr >> 24);
    term_putbyte(phdr->p_vaddr >> 16);
    term_putbyte(phdr->p_vaddr >> 8);
    term_putbyte(phdr->p_vaddr);
    term_print(" from \n 0x");
    term_putbyte(phdr->p_offset >> 8);
    term_putbyte(phdr->p_offset);
    term_print("\n\n");
  }

  while(1){}
}

