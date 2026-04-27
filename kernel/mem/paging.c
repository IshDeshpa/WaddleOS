#include "paging.h"
#include "interrupts.h"
#include "multiboot2.h"
#include <stddef.h>
#include <stdint.h>
#include "string.h"
#include "bitmap.h"
#include "utils.h"
#include "multiboot_parser.h"

#define LOG_LEVEL 5
#define LOG_ENABLE 1
#include "log.h"

static struct multiboot_mmap_entry memory_map[10];
static int memory_map_len = 0;

static uint64_t pool_pages_total;
static uintptr_t phys_mem_end_paddr;
extern char _kernel_end;

static p4e_t *init_pml4 = NULL;

typedef struct {
  bitmap_t allocated_bmp;
  bitmap_t pinned_bmp; // protected from eviction
  void *md_base;
  void *page_base;
} pool_t;

static pool_t kernel_pool;
static pool_t user_pool;

static inline void *k_ptov(void *phys_addr){
  return (void *)((uintptr_t)phys_addr + PHYS_BASE);
}

static inline void *k_vtop(void *virt_addr){
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
  for(int i=0; i<memory_map_len; i++){
    log(LOG_TRACE, "%d: %x - %x\n", i, memory_map[i].addr, memory_map[i].addr + memory_map[i].len);
    log(LOG_TRACE, "\t%d\n", memory_map[i].type);
  }

  phys_mem_end_paddr = memory_map[memory_map_len - 1].addr + memory_map[memory_map_len - 1].len; // subtract 1MiB from memory size allocated for random bootloader crap
  log(LOG_TRACE, "phys_mem_end_paddr: %x\n", phys_mem_end_paddr);
}

// Make sure pool_base (base address of the bitmaps) and page_base (base address of the pages) don't conflict!
static void memory_pool_init(void *pool_base, void *page_base, size_t pool_pages, pool_t *pool_struct){
  size_t bmp_bytes = ROUND_UP_TO(pool_pages, 8) / 8; // number of bytes required to store pool_pages number of bits
  size_t md_pages = ROUND_UP_TO(2 * bmp_bytes, PAGE_SIZE) / PAGE_SIZE; // number of pages required to store bmp_bytes number of bytes (*2 for both bmps metadata)
  
  pool_struct->md_base = pool_base;
  pool_struct->page_base = page_base;

  bitmap_init_buf(&pool_struct->allocated_bmp, pool_base, pool_pages);
  bitmap_init_buf(&pool_struct->pinned_bmp, (void*)((uintptr_t)pool_base + bmp_bytes), pool_pages);

  // Mark bitmap pages as allocated and reserved
  for(size_t i=0; i<md_pages; i++){
    bitmap_set(&pool_struct->allocated_bmp, i, true);
    bitmap_set(&pool_struct->pinned_bmp, i, true);
  }

  // Mark reserved memory sections as protected, if any are in this pool region
  for(int i=0; i<memory_map_len; i++){
    struct multiboot_mmap_entry *entry = &memory_map[i];

    if(entry->type != MULTIBOOT_MEMORY_AVAILABLE){
      // Check intersection
      uintptr_t pool_start = (uintptr_t)pool_struct->md_base;
      uintptr_t pool_end = pool_start + pool_pages*PAGE_SIZE;

      uintptr_t region_start = (uintptr_t)k_ptov((void*)entry->addr);
      uintptr_t region_end = (uintptr_t)k_ptov((void*)(entry->addr + entry->len));

      uintptr_t overlap_start = (region_start > pool_start)?region_start:pool_start;
      uintptr_t overlap_end = (region_end < pool_end)?region_end:pool_end;

      if(overlap_start < overlap_end){
        uint64_t start_page = (overlap_start - pool_start)/PAGE_SIZE;
        uint64_t end_page = (overlap_end - pool_start)/PAGE_SIZE;

        for(uint64_t j=start_page; j<end_page; j++){
          bitmap_set(&pool_struct->allocated_bmp, j, true);
          bitmap_set(&pool_struct->pinned_bmp, j, true);
        }
      }
    }
  }
}

static inline void *pde_init(pde_t *entry, bool allow_user_access){
  void *v_base;
  void *p_base;
  if (GET_BITS(*entry, PDE_PTE_PRESENT_POS, PDE_PTE_PRESENT_LEN) == 0){
    // Doesn't exist yet
    v_base = paging_get_page(PALLOC_ZERO);
    p_base = k_vtop(v_base);
    log(LOG_TRACE, "Allocating new page for PDE...\n");

    // Clear
    memset(entry, 0, sizeof(pde_t));
    *entry = SET_BITS(*entry, PDE_PTE_BASE_POS, PDE_PTE_BASE_LEN, (uintptr_t)p_base >> 12); // base address
    *entry = SET_BITS(*entry, PDE_PTE_US_POS, PDE_PTE_US_LEN, allow_user_access); // user/supervisor
    *entry = SET_BITS(*entry, PDE_PTE_RW_POS, PDE_PTE_RW_LEN, 1); // rw
    *entry = SET_BITS(*entry, PDE_PTE_PRESENT_POS, PDE_PTE_PRESENT_LEN, 1); // present
  } else{
    // Already exists
    p_base = (void *)(GET_BITS(*entry, PDE_PTE_BASE_POS, PDE_PTE_BASE_LEN) << 12);
    v_base = k_ptov(p_base);
    log(LOG_TRACE, "Using existing page for PDE... %x\n", p_base);
  }

  return v_base;
}

static inline void pte_init(pte_t *entry, void *phys_addr, bool allow_user_access){
  // Clear
  memset(entry, 0, sizeof(pte_t));

  *entry = SET_BITS(*entry, PDE_PTE_BASE_POS, PDE_PTE_BASE_LEN, (uintptr_t)phys_addr >> 12); // physical address

  *entry = SET_BITS(*entry, PDE_PTE_US_POS, PDE_PTE_US_LEN, allow_user_access); // user/supervisor
  *entry = SET_BITS(*entry, PDE_PTE_RW_POS, PDE_PTE_RW_LEN, 1); // rw
  *entry = SET_BITS(*entry, PDE_PTE_PRESENT_POS, PDE_PTE_PRESENT_LEN, 1); // present
}

static void paging_map_page(void *vaddr, void *paddr){
    uint32_t pml4_ofs = GET_PML4_OFS((uintptr_t)vaddr);
    p3e_t *pml3 = pde_init(&init_pml4[pml4_ofs], false);
    log(LOG_TRACE, "PML4 Offset: %d\n", pml4_ofs);
    log(LOG_TRACE, "PML3 Address: %x\n", pml3);

    uint32_t pml3_ofs = GET_PML3_OFS((uintptr_t)vaddr);
    p2e_t *pml2 = pde_init(&pml3[pml3_ofs], false);
    log(LOG_TRACE, "PML3 Offset: %d\n", pml3_ofs);
    log(LOG_TRACE, "PML2 Address: %x\n", pml2);

    uint32_t pml2_ofs = GET_PML2_OFS((uintptr_t)vaddr);
    p1e_t *pml1 = pde_init(&pml2[pml2_ofs], false);
    log(LOG_TRACE, "PML2 Offset: %d\n", pml2_ofs);
    log(LOG_TRACE, "PML1 Address: %x\n", pml1);

    uint32_t pml1_ofs = GET_PML1_OFS((uintptr_t)vaddr);
    pte_init(&pml1[pml1_ofs], paddr, false);
    log(LOG_TRACE, "PML1 Offset: %d\n", pml1_ofs);
}

static void page_tables_init(){
  init_pml4 = paging_get_page(PALLOC_ZERO);

  // Map all pages to higher half
  for(uintptr_t page = 0; page < pool_pages_total; page++){
    void *phys_addr = (void *)(page * PAGE_SIZE);
    log(LOG_TRACE, "Physical address: %x\n", phys_addr);

    void *virt_addr = k_ptov(phys_addr);
    log(LOG_TRACE, "Virtual address: %x\n", virt_addr);
    
    paging_map_page(virt_addr, phys_addr);
  }
  
  // convert to phys addr to put in CR3
  void *init_pml4_cr3 = k_vtop(init_pml4);

  // Load CR3
  __asm__ volatile (
    "movq %0, %%cr3"    // Move the 64-bit value into CR3.
    :                   // No outputs
    : "r" (init_pml4_cr3) // Input: PML4 address from a register
    : "memory"          // Clobbered: TLB is flushed
  );
}

static void page_fault_handler(interrupt_frame_t *frame){
  (void)frame;

  uintptr_t fault_addr;
  asm volatile ("mov %%cr2, %0" : "=r"(fault_addr));

  log(LOG_TRACE, "Page fault! %x\n", fault_addr);
}

void paging_init(){
  memory_map_init();

  interrupts_register_handler(EXC_PAGE_FAULT, page_fault_handler);
  
  // we know that the kernel is currently loaded at 1MiB, so we want to start both memory pools past the kernel end
  uintptr_t free_start_vaddr = ROUND_UP_TO((uintptr_t)&_kernel_end, PAGE_SIZE);
  log(LOG_TRACE, "free_pool_start_vaddr: %x\n", free_start_vaddr);

  pool_pages_total = ((uintptr_t)(k_ptov((void *)phys_mem_end_paddr) - free_start_vaddr))/PAGE_SIZE;
  log(LOG_TRACE, "pool_pages_total: %d\n", pool_pages_total);

  // Initialize the memory pools
  size_t k_pool_pages = pool_pages_total/2;
  size_t u_pool_pages = pool_pages_total - pool_pages_total/2;
  uintptr_t k_bmp_start = free_start_vaddr;
  uintptr_t u_bmp_start = free_start_vaddr + (ROUND_UP_TO(k_pool_pages, 8)/8) * 2;
  uintptr_t k_bmp_tracking_start = free_start_vaddr;
  uintptr_t u_bmp_tracking_start = free_start_vaddr + k_pool_pages*PAGE_SIZE;

  memory_pool_init((void*)k_bmp_start, (void*)k_bmp_tracking_start, k_pool_pages, &kernel_pool);
  memory_pool_init((void*)u_bmp_start, (void*)u_bmp_tracking_start, u_pool_pages, &user_pool);

  // Pre-map all of RAM
  page_tables_init();

  // Debug prints
  log(LOG_DEBUG, "_kernel_end: %x\n", free_start_vaddr);
  log(LOG_DEBUG, "k_bmp_start: %x\n", kernel_pool.md_base);
  log(LOG_DEBUG, "u_bmp_start: %x\n", user_pool.md_base);
  log(LOG_DEBUG, "k_bmp_tracking_start: %x\n", kernel_pool.page_base);
  log(LOG_DEBUG, "u_bmp_tracking_start: %x\n", user_pool.page_base);
}

void *paging_get_pages(uint64_t num_pages, uint8_t flags){
  pool_t *mem_pool = &kernel_pool;
  if (flags & PALLOC_USER) {
    mem_pool = &user_pool;
  }

  int64_t pg_ind = bitmap_test_and_flip(&mem_pool->allocated_bmp, false, 0, mem_pool->allocated_bmp.size - 1, num_pages);
  if(pg_ind == -1) return NULL;

  void *pg_ptr = mem_pool->page_base + pg_ind*PAGE_SIZE;

  log(LOG_TRACE, "Page index allocated: %d\n", pg_ind);
  
  if (flags & PALLOC_ZERO){
    memset(pg_ptr, 0, num_pages * PAGE_SIZE);
  }

  return pg_ptr;
}

void *paging_get_page(uint8_t flags){
  return paging_get_pages(1, flags);
}

