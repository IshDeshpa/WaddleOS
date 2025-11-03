/**
 * https://www.cs.cmu.edu/~410-s22/p4/p4-boot.pdf
 * https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html
 */

#include "multiboot2.h"
#include <stdint.h>
#include "string.h"
#include "serial.h"
#include "printf.h"

#include "elf.h"
#include "constants.h"

int check_for_cpuid(void);
int check_for_longmode(void);
void long_mode(void);

extern Elf64_Addr _kernel_entry;

// Num_sectors must be <= 24 and sector must be >= 12 for load_sector_asm
void load_sector_asm(uint16_t sector, uint16_t num_sectors);
void load_sector(uint16_t sector, uint16_t num_sectors, uint8_t* address){
  while(num_sectors > 0){
    uint16_t sectors_now = (num_sectors > 24)?24:num_sectors;
    
    load_sector_asm(sector, sectors_now); // loads num sectors starting at sector into 0x8000
    // Memcopy
    memcpy(address, ((const void*)DISK_BUFFER_ADDR), (sectors_now << 9));

    sector += sectors_now;
    address += (sectors_now << 9);
    num_sectors -= sectors_now;
  }
}

void get_basic_meminfo(uint32_t *mem_lower, uint32_t *mem_upper);
uint32_t get_single_mmap(struct multiboot_mmap_entry *mmap_entry);

static Elf64_Ehdr local_ehdr;

typedef struct {
  size_t bytes;
  uint8_t *addr;
  size_t offset;
} load_info_t;

int main(){
  serial_init(COM1);

  // Load a sector into disk buffer
  load_sector_asm(KERNEL_START_SECTOR, 1);

  // Cast to ELF64 header (0x40 bytes long)
  Elf64_Ehdr *e_header = (Elf64_Ehdr *)DISK_BUFFER_ADDR;
  memcpy(&local_ehdr, e_header, sizeof(Elf64_Ehdr));
    
  // Confirm ELF magic number
  if(memcmp(e_header->e_ident, ELFMAG, SELFMAG) != 0){
    printf(DEV_SERIAL_COM1, "ELF magic incorrect!\n");
    while(1);
  }

  printf(DEV_SERIAL_COM1, "Elf header obtained!\n");

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

      printf(DEV_SERIAL_COM1, "Linfo[%d] = %x\n\r", j, linfo[j].addr);
      j++;
    }
  }

  // Load first section
  load_sector(KERNEL_START_SECTOR + (linfo[0].offset>>9), ((linfo[0].bytes>>9) + 1), linfo[0].addr);

  // MULTIBOOT TIME
  struct multiboot_header *mhdr = (struct multiboot_header *)linfo[0].addr;
  
  // Verify magic number
  if(mhdr->magic != MULTIBOOT2_HEADER_MAGIC){
    while(1) {}
  }

  // Verify checksum
  if((mhdr->magic + mhdr->architecture + mhdr->header_length + mhdr->checksum) != 0){
    while(1) {}
  }

  // Check for tag requests
  struct multiboot_header_tag *tag = (struct multiboot_header_tag *)((uint8_t*)mhdr + sizeof(struct multiboot_header));
  volatile uint32_t tags_requested_bmap = 0;

  while(tag->type != MULTIBOOT_HEADER_TAG_END){
    switch(tag->type){
      case MULTIBOOT_HEADER_TAG_INFORMATION_REQUEST:
        struct multiboot_header_tag_information_request *t = (struct multiboot_header_tag_information_request*)tag;
        for(uint8_t i=0; i<(t->size - 8)/4; i++){
          tags_requested_bmap |= (1<<t->requests[i]);
        }
        break;
      default: break;
    }
    tag = (struct multiboot_header_tag *)((uintptr_t)tag + tag->size);
    // align for next tag
    tag = (struct multiboot_header_tag *)(((uintptr_t)tag + 7) & ((uintptr_t)~7));
  }

  printf(DEV_SERIAL_COM1, "Tags Requested: %d\n", tags_requested_bmap);

  // Loop through requested tags and return info
  uint32_t *multiboot_return = (uint32_t *)MULTIBOOT_RETURN_INFO_ADDR;
  multiboot_return[0] = 2;// total_size
  multiboot_return[1] = 0;// reserved
  
  struct multiboot_tag *multiboot_return_curr = (struct multiboot_tag *)(multiboot_return + 2);

  uint8_t curr_tag_type = 0;

  while(tags_requested_bmap != 0){
    if(tags_requested_bmap & 0x1){
      switch(curr_tag_type){
        case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
          struct multiboot_tag_basic_meminfo *t = (struct multiboot_tag_basic_meminfo *)multiboot_return_curr;
          get_basic_meminfo((uint32_t*)&t->mem_lower, (uint32_t*)&t->mem_upper);
          multiboot_return_curr->size = sizeof(struct multiboot_tag_basic_meminfo);
          break;
        case MULTIBOOT_TAG_TYPE_MMAP:
          struct multiboot_tag_mmap *t2 = (struct multiboot_tag_mmap *)multiboot_return_curr;
          struct multiboot_mmap_entry *curr_entry = t2->entries;
          while(get_single_mmap(curr_entry) != 1){
            curr_entry++;
          }
          t2->entry_size = 24;
          multiboot_return_curr->size = (uintptr_t)curr_entry - (uintptr_t)t2;
          t2->entry_version = 0;
          break;
        default: break;
      }
      multiboot_return_curr->type = curr_tag_type;
    }

    tags_requested_bmap >>= 1;
    curr_tag_type++;
    multiboot_return[0] += multiboot_return_curr->size;
    multiboot_return_curr = (struct multiboot_tag *)(((uintptr_t)multiboot_return_curr + multiboot_return_curr->size + 7) & ((uintptr_t)~7));
  }

  // Load remaining sections
  for(uint16_t i=0; i<j; i++){
    load_sector(KERNEL_START_SECTOR + (linfo[i].offset>>9), ((linfo[i].bytes>>9) + 1), linfo[i].addr);
  }

  // Check for CPUID
  if(check_for_cpuid() != 1){
    printf(DEV_SERIAL_COM1, "No CPUID!\n");
  }

  // Check for CPUID extensions and check for long mode
  if(check_for_longmode() != 1){
    printf(DEV_SERIAL_COM1, "No long mode!\n");
    while(1) {}
  }

  printf(DEV_SERIAL_COM1, "Entering kernel at %x\n", local_ehdr.e_entry);
  _kernel_entry = local_ehdr.e_entry;

  long_mode();

  while(1){}
}

