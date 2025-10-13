#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

typedef struct {
  uint8_t xd : 1; // execute disable
  uint16_t avl1: 10; // available to software
  uint64_t base : 39; // page-directory pointer base address
  uint8_t avl2: 3; // available to software
  uint8_t mbz: 3; // must be zero
  uint8_t acc: 1; // accessed
  uint8_t pcd: 1; // page-level cache disable
  uint8_t pwt: 1; // page-level writethrough
  uint8_t us: 1; // user/supervisor (both is 1, supervisor only is 0)
  uint8_t rw: 1; // read/write
  uint8_t present: 1; // present
} __attribute__((packed)) pml4e_t; // page-map level 4 entry

typedef struct {
  uint8_t xd : 1; // execute disable
  uint16_t avl1: 10; // available to software
  uint64_t base : 39; // page-directory base address
  uint8_t avl2: 3; // available to software
  uint8_t mbz: 3; // must be zero
  uint8_t acc: 1; // accessed
  uint8_t pcd: 1; // page-level cache disable
  uint8_t pwt: 1; // page-level writethrough
  uint8_t us: 1; // user/supervisor (both is 1, supervisor only is 0)
  uint8_t rw: 1; // read/write
  uint8_t present: 1; // present
} __attribute__((packed)) pdpe_t; // page-directory pointer entry

typedef struct {
  uint8_t xd : 1; // execute disable
  uint16_t avl1: 10; // available to software
  uint64_t base : 39; // page-table base address
  uint8_t avl2: 3; // available to software
  uint8_t mbz: 3; // must be zero
  uint8_t acc: 1; // accessed
  uint8_t pcd: 1; // page-level cache disable
  uint8_t pwt: 1; // page-level writethrough
  uint8_t us: 1; // user/supervisor (both is 1, supervisor only is 0)
  uint8_t rw: 1; // read/write
  uint8_t present: 1; // present
} __attribute__((packed)) pde_t; // page-directory entry

typedef struct {
  uint8_t xd : 1; // execute disable
  uint8_t mpk : 4; // memory protection key bits (depends on CR4.PKE, otherwise are available)
  uint16_t avl1: 10; // available to software
  uint64_t base : 39; // page base address
  uint8_t avl2: 3; // available to software
  uint8_t g: 1; // global page (should the page translation entry be invalidated when switching CR3)
  uint8_t pat: 1; // page attribute table bit
  uint8_t dir: 1; // dirty
  uint8_t acc: 1; // accessed
  uint8_t pcd: 1; // page-level cache disable
  uint8_t pwt: 1; // page-level writethrough
  uint8_t us: 1; // user/supervisor (both is 1, supervisor only is 0)
  uint8_t rw: 1; // read/write
  uint8_t present: 1; // present
} __attribute__((packed)) pte_t; // page-directory entry

typedef pml4e_t p4e_t;
typedef pdpe_t p3e_t;
typedef pde_t p2e_t;
typedef pte_t p1e_t;

void paging_init();

#endif
