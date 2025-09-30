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

int check_for_cpuid(void);
int check_for_longmode(void);
void long_mode(void);

extern Elf64_Addr _kernel_entry;

// Num_sectors must be <= 24 and sector must be >= 12
void load_sector_asm(uint16_t sector, uint16_t num_sectors);
void load_sector(uint16_t sector, uint16_t num_sectors, uint8_t* address){
  load_sector_asm(sector, num_sectors); // loads num sectors starting at sector into 0x8000
  // Memcopy
  memcpy(address, ((const void*)DISK_BUFFER_ADDR), ((size_t)num_sectors << 9));

#ifdef DEBUG
  // Print out first 16 bytes
  //for(uint8_t *i=(uint8_t*)DISK_BUFFER_ADDR; i<((uint8_t*)DISK_BUFFER_ADDR + 16); i++){
  //  term_putbyte(*(i) & 0xFF);
  //  term_putc('\n');
  //}
#endif
}

static Elf64_Ehdr local_ehdr;

typedef struct {
  size_t bytes;
  uint8_t *addr;
  size_t offset;
} load_info_t;

int main(){
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
  load_info_t linfo[5];
  uint16_t j=0;
  for(uint16_t i=0; i<local_ehdr.e_phnum; i++){
    load_sector_asm(KERNEL_START_SECTOR + ((i * local_ehdr.e_phentsize) >> 9), (local_ehdr.e_phentsize>>9)+2);

    Elf64_Phdr *phdr = (Elf64_Phdr *)(DISK_BUFFER_ADDR + ((local_ehdr.e_phoff + i * local_ehdr.e_phentsize)%512));

    if(phdr->p_type == PT_LOAD){
      linfo[j].bytes = phdr->p_filesz;
      linfo[j].addr = (uint8_t *)phdr->p_paddr;
      linfo[j].offset = phdr->p_offset;

#ifdef DEBUG
      term_print("Load\n 0x");
      term_putbyte(phdr->p_filesz >> 8);
      term_putbyte(phdr->p_filesz);
      term_print(" bytes to \n 0x");
      term_putbyte(phdr->p_paddr>> 24);
      term_putbyte(phdr->p_paddr >> 16);
      term_putbyte(phdr->p_paddr >> 8);
      term_putbyte(phdr->p_paddr);
      term_print(" from \n 0x");
      term_putbyte(phdr->p_offset>>8);
      term_putbyte(phdr->p_offset);
      term_print("\n\n");
#endif
      j++;
    }
  }

  // Load proper sections
  for(uint16_t i=0; i<j; i++){
    load_sector(KERNEL_START_SECTOR + (linfo[i].offset>>9), ((linfo[i].bytes>>9) + 1), linfo[i].addr);
  }

  // Check for CPUID
  if(check_for_cpuid() != 1){
    term_print("No CPUID!\n");
  }

  // Check for CPUID extensions and check for long mode
  if(check_for_longmode() != 1){
    term_print("No long mode!\n");
    while(1) {}
  }

  term_print("Entering kernel...\n");
  _kernel_entry = local_ehdr.e_entry;

  long_mode();

  while(1){}
}

