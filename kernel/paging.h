#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define GET_PML4_OFS(x) (((x) >> 39) & 0x1FF)
#define GET_PML3_OFS(x) (((x) >> 30) & 0x1FF)
#define GET_PML2_OFS(x) (((x) >> 21) & 0x1FF)
#define GET_PML1_OFS(x) (((x) >> 12) & 0x1FF)
#define GET_OFS(x) (x & 0xFFF)

#define PHYS_BASE (0xFFFFFFFF80000000)

#define PAGE_SIZE (4096)

#define GET_BITS(value, pos, len) ((((uint64_t)(value)) >> (pos)) & ((1ULL << (len)) - 1))
#define SET_BITS(value, pos, len, newval) \
    ((((uint64_t)(value)) & ~(((1ULL << (len)) - 1) << (pos))) | \
    ((((uint64_t)(newval)) & ((1ULL << (len)) - 1)) << (pos)))

// execute disable
#define PDE_PTE_XD_POS (63) 
#define PDE_PTE_XD_LEN (1)

// available to software
#define PDE_PTE_AVL1_POS (52)
#define PDE_AVL1_LEN (11)
#define PTE_AVL1_LEN (7)

// next page-directory base address
#define PDE_PTE_BASE_POS (12)
#define PDE_PTE_BASE_LEN (39)

// available to software
#define PDE_PTE_AVL2_POS (9)
#define PDE_PTE_AVL2_LEN (3)

// must be zero
#define PDE_MBZ_POS (6)
#define PDE_MBZ_LEN (3)

// accessed
#define PDE_PTE_ACC_POS (5)
#define PDE_PTE_ACC_LEN (1)

// page-level cache disable
#define PDE_PTE_PCD_POS (4)
#define PDE_PTE_PCD_LEN (1)

// page-level writethrough
#define PDE_PTE_PWT_POS (3)
#define PDE_PTE_PWT_LEN (1)

// user/supervisor (both is 1, supervisor only is 0)
#define PDE_PTE_US_POS (2)
#define PDE_PTE_US_LEN (1)

// read/write
#define PDE_PTE_RW_POS (1)
#define PDE_PTE_RW_LEN (1)

// present
#define PDE_PTE_PRESENT_POS (0)
#define PDE_PTE_PRESENT_LEN (1)

// memory protection key bits (depends on CR4.PKE, otherwise are available)
#define PTE_MPK_POS (59)
#define PTE_MPK_LEN (4)

// global page (should the page translation entry be invalidated when switching CR3)
#define PTE_GLOB_POS (8)
#define PTE_GLOB_LEN (1)

// page attribute table bit
#define PTE_PAT_POS (7)
#define PTE_PAT_LEN (1)

// dirty bit
#define PTE_DIRTY_POS (6)
#define PTE_DIRTY_LEN (1)

typedef uint64_t p4e_t;
typedef uint64_t p3e_t;
typedef uint64_t p2e_t;
typedef uint64_t p1e_t;

typedef uint64_t pde_t;
typedef p1e_t pte_t;

typedef enum {
  PALLOC_USER = 0x1,
  PALLOC_ZERO = 0x2,
} palloc_flags_t;

void paging_init();
void *paging_get_page(uint8_t flags);

#endif
