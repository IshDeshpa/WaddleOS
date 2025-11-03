#include "paging.h"
#include "interrupts.h"
#include "multiboot2.h"
#include <stddef.h>
#include "string.h"
#include "bitmap.h"
#include "multiboot_parser.h"

#define LOG_LEVEL 0
#define LOG_ENABLE 0
#include "log.h"

static struct multiboot_mmap_entry memory_map[10];
static int memory_map_len = 0;

uint64_t pool_pages_total;
static uintptr_t phys_mem_end;
extern char _kernel_end;

static p4e_t *init_pml4 = NULL;

typedef struct {
  bitmap_t allocated_bmp;
  bitmap_t reserved_bmp; // protected from eviction
  void *md_base;
  void *page_base;
} pool_t;

static pool_t kernel_pool;
static pool_t user_pool;

static inline void *phys_to_virt(void *phys_addr){
  return (void *)((uintptr_t)phys_addr + PHYS_BASE);
}

static inline void *virt_to_phys(void *virt_addr){
  return (void *)((uintptr_t)virt_addr - PHYS_BASE);
}

static void memory_map_init(){
  struct multiboot_tag_mmap *mmap_tag = (struct multiboot_tag_mmap *)multiboot_get_tag(MULTIBOOT_TAG_TYPE_MMAP);
  struct multiboot_mmap_entry *entries = mmap_tag->entries;
 
  int mmap_size = mmap_tag->size - 24;
  memory_map_len = mmap_size/mmap_tag->entry_size;

  // Copy to global array
  memcpy(memory_map, entries, (mmap_tag->size - 24));

  // Sort
  for(int i=1; i<memory_map_len; i++){
    int j=i;
    while(j > 0 && memory_map[j].addr < memory_map[j-1].addr){
      struct multiboot_mmap_entry tmp = memory_map[j-1];
      memory_map[j-1] = memory_map[j];
      memory_map[j] = tmp;
      j--;
    }
  }

  // Print
  //#ifdef DEBUG
  for(int i=0; i<memory_map_len; i++){
    log(LOG_TRACE, "\t%x-%x\n\r", memory_map[i].addr, memory_map[i].len);
    log(LOG_TRACE, "\t\t%d\n\r", memory_map[i].type);
  }
  //#endif

  phys_mem_end = memory_map[memory_map_len - 1].addr + memory_map[memory_map_len - 1].len; // subtract 1MiB from memory size allocated for random bootloader crap
}

static void memory_pool_init(void *pool_base, size_t pool_pages, pool_t *pool_struct){
  uint8_t md_pages = ((pool_pages * 2) + 4095)/4096; // 2 bits per page, rounded up to nearest page bound
  pool_struct->md_base = pool_base;
  pool_struct->page_base = pool_base + md_pages*4096;
  bitmap_init_buf(&pool_struct->allocated_bmp, pool_base, pool_pages);
  bitmap_init_buf(&pool_struct->reserved_bmp, pool_base+(pool_pages+7)/8, pool_pages);

  // Mark bitmap pages as allocated and reserved
  for(int i=0; i<md_pages; i++){
    bitmap_set(&pool_struct->allocated_bmp, i, true);
    bitmap_set(&pool_struct->reserved_bmp, i, true);
  }

  // Mark reserved memory sections as protected, if any are in this pool region
  for(int i=0; i<memory_map_len; i++){
    struct multiboot_mmap_entry *entry = &memory_map[i];

    if(entry->type != MULTIBOOT_MEMORY_AVAILABLE){
      // Check intersection
      uintptr_t pool_start = (uintptr_t)pool_struct->md_base;
      uintptr_t pool_end = pool_start + pool_pages*4096;

      uintptr_t region_start = (uintptr_t)phys_to_virt((void*)entry->addr);
      uintptr_t region_end = (uintptr_t)phys_to_virt((void*)(entry->addr + entry->len));

      uintptr_t overlap_start = (region_start > pool_start)?region_start:pool_start;
      uintptr_t overlap_end = (region_end < pool_end)?region_end:pool_end;

      if(overlap_start < overlap_end){
        uint64_t start_page = (overlap_start - pool_start)/4096;
        uint64_t end_page = (overlap_end - pool_start)/4096;

        for(uint64_t i=start_page; i<end_page; i++){
          bitmap_set(&pool_struct->allocated_bmp, i, true);
          bitmap_set(&pool_struct->reserved_bmp, i, true);
        }
      }
    }
  }
}

static inline void *pde_init(pde_t *entry, bool allow_user_access){
  void *base;
  if (GET_BITS(*entry, PDE_PTE_PRESENT_POS, PDE_PTE_PRESENT_LEN) == 0){
    base = virt_to_phys(paging_get_page(PALLOC_ZERO));
    log(LOG_TRACE, "Allocating new page for PDE...\n\r");
  } else{
    base = (void *)(GET_BITS(*entry, PDE_PTE_BASE_POS, PDE_PTE_BASE_LEN) << 12);
    log(LOG_TRACE, "Using existing page for PDE... %x\n\r", base);
  }

  // Clear
  memset(entry, 0, sizeof(pde_t));

  *entry = SET_BITS(*entry, PDE_PTE_BASE_POS, PDE_PTE_BASE_LEN, (uintptr_t)base >> 12); // base address

  *entry = SET_BITS(*entry, PDE_PTE_US_POS, PDE_PTE_US_LEN, allow_user_access); // user/supervisor
  *entry = SET_BITS(*entry, PDE_PTE_RW_POS, PDE_PTE_RW_LEN, 1); // rw
  *entry = SET_BITS(*entry, PDE_PTE_PRESENT_POS, PDE_PTE_PRESENT_LEN, 1); // present
  
  return base;
}

static inline void pte_init(pte_t *entry, void *phys_addr, bool allow_user_access){
  // Clear
  memset(entry, 0, sizeof(pte_t));

  *entry = SET_BITS(*entry, PDE_PTE_BASE_POS, PDE_PTE_BASE_LEN, (uintptr_t)phys_addr >> 12); // physical address

  *entry = SET_BITS(*entry, PDE_PTE_US_POS, PDE_PTE_US_LEN, allow_user_access); // user/supervisor
  *entry = SET_BITS(*entry, PDE_PTE_RW_POS, PDE_PTE_RW_LEN, 1); // rw
  *entry = SET_BITS(*entry, PDE_PTE_PRESENT_POS, PDE_PTE_PRESENT_LEN, 1); // present
}

void paging_map_page(void *vaddr, void *paddr){
    uint32_t pml4_ofs = GET_PML4_OFS((uintptr_t)vaddr);
    p3e_t *pml3 = pde_init(&init_pml4[pml4_ofs], false);
    log(LOG_TRACE, "PML4 Offset: %d\n\r", pml4_ofs);
    log(LOG_TRACE, "PML3 Address: %x\n\r", pml3);

    uint32_t pml3_ofs = GET_PML3_OFS((uintptr_t)vaddr);
    p2e_t *pml2 = pde_init(&pml3[pml3_ofs], false);
    log(LOG_TRACE, "PML3 Offset: %d\n\r", pml3_ofs);
    log(LOG_TRACE, "PML2 Address: %x\n\r", pml2);

    uint32_t pml2_ofs = GET_PML2_OFS((uintptr_t)vaddr);
    p1e_t *pml1 = pde_init(&pml2[pml2_ofs], false);
    log(LOG_TRACE, "PML2 Offset: %d\n\r", pml2_ofs);
    log(LOG_TRACE, "PML1 Address: %x\n\r", pml1);

    uint32_t pml1_ofs = GET_PML1_OFS((uintptr_t)vaddr);
    pte_init(&pml1[pml1_ofs], paddr, false);
    log(LOG_TRACE, "PML1 Offset: %d\n\r", pml1_ofs);
}

static void page_tables_init(){
  init_pml4 = paging_get_page(PALLOC_ZERO);

  // Map only active kernel code pages and pool metadata pages to higher half, starting at 0x0
  for(uint64_t page = 0; page < ((uintptr_t)virt_to_phys(kernel_pool.page_base))/4096; page++){
    void *phys_addr = (void *)(page * PAGE_SIZE);
    log(LOG_TRACE, "Physical address: %x\n\r", phys_addr);

    void *virt_addr = phys_to_virt(phys_addr);
    log(LOG_TRACE, "Virtual address: %x\n\r", virt_addr);
    
    paging_map_page(virt_addr, phys_addr);
  }
  
  // convert to phys addr to put in CR3
  void *init_pml4_cr3 = virt_to_phys(init_pml4);

  // Load CR3
  __asm__ volatile (
    "movq %0, %%cr3"    // Move the 64-bit value into CR3.
    :                   // No outputs
    : "r" (init_pml4_cr3) // Input: PML4 address from a register
    : "memory"          // Clobbered: TLB is flushed
  );
}

static void page_fault_handler(interrupt_frame_t *frame){
  uintptr_t fault_addr;
  asm volatile ("mov %%cr2, %0" : "=r"(fault_addr));

  log(LOG_TRACE, "Page fault! %x\n\r", fault_addr);
}

void paging_init(){
  memory_map_init();

  interrupts_register_handler(EXC_PAGE_FAULT, page_fault_handler);
  
  // we know that the kernel is currently loaded at 1MiB, so we want to start the kernel pool past the kernel end
  uintptr_t free_pool_start = ((uintptr_t)&_kernel_end + 0xFFF) & (~0xFFF);
  log(LOG_TRACE, "Free pool start: %x\n\r", free_pool_start);

  pool_pages_total = (phys_mem_end - free_pool_start)/4096;

  memory_pool_init((void *)free_pool_start, pool_pages_total/2, &kernel_pool);
  memory_pool_init((void *)(free_pool_start + pool_pages_total/2), pool_pages_total - pool_pages_total/2, &user_pool);

  // initialize page tables
  page_tables_init();
}

void *paging_get_page(uint8_t flags){
  pool_t *mem_pool = &kernel_pool;
  if (flags & PALLOC_USER) {
    mem_pool = &user_pool;
  }

  uint64_t pg_ind = bitmap_test_and_flip(&mem_pool->allocated_bmp, false); 
  void *pg_ptr = pg_ind*4096 + mem_pool->md_base;

  log(LOG_TRACE, "Page index allocated: %d\n\r", pg_ind);
  
  if (flags & PALLOC_ZERO){
    memset(pg_ptr, 0, 4096);
  }

  return pg_ptr;
}

// Will only consider vaddr if a user page; kernel pages will auto-map to their proper location in the linear mapping
void *paging_get_and_map_page(uint8_t flags, void *vaddr){
  void *paddr = virt_to_phys(paging_get_page(flags));
  if (flags & PALLOC_USER){
    paging_map_page(vaddr, paddr);
  } else {
    paging_map_page(vaddr, virt_to_phys(vaddr));
  }
}
