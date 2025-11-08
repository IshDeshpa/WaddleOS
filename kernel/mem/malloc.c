#include "malloc.h"
#include <stddef.h>
#include <stdint.h>
#include "utils.h"

#include "log.h"

typedef struct free_hdr free_hdr_t;
struct free_hdr {
  int64_t sz; // minimum alloc size is 8 bytes, bottom bit is 1 if alloc and 0 if not
  free_hdr_t *next_free;
  free_hdr_t *prev_free;
};

typedef struct {
  int64_t sz;
} alloc_hdr_t;

static free_hdr_t *freelist_head;

void malloc_init(void *start_addr, size_t size){
  // Establish initial header
  freelist_head = start_addr;
  freelist_head->sz = size - sizeof(free_hdr_t);
  freelist_head->next_free = NULL;
  freelist_head->prev_free = NULL;
}

void *malloc(size_t size){
  // Round up size
  size = (size + 7) & ~0x7;

  free_hdr_t *curr = freelist_head;
  free_hdr_t *min = freelist_head;
  
  // Find best-fit
  while(curr->next_free != NULL){
    curr = curr->next_free;

    if(curr->sz >= size && 
      curr->sz < min->sz){
      min = curr;
    }
  }

  ASSERT(min->sz >= size);

  // Split
  free_hdr_t *next = min->next_free;
  free_hdr_t *prev = min->prev_free;

  free_hdr_t *new_next = (free_hdr_t *)((uint8_t *)min + sizeof(alloc_hdr_t) + size);
  new_next->sz = min->sz - size - sizeof(alloc_hdr_t) - sizeof(free_hdr_t);
  new_next->next_free = next;
  new_next->prev_free = prev;

  if(next) next->prev_free = new_next;
  if(prev) prev->next_free = new_next;

  // Allocate
  alloc_hdr_t *ret = (alloc_hdr_t *)min;
  ret->sz = size | 0x1;

  return (void *)ret;
}

void *calloc(size_t num, size_t size){

}

void *realloc(void* ptr, size_t size){

}

void *free(void* ptr){

}
