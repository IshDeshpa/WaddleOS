#include "wdmalloc.h"
#include <stddef.h>
#include <stdint.h>
#include "utils.h"

#define WDM_ALLOC_BIT (1 << 15)
#define WDM_MAGIC (0xDEADBEEFBABECAFE)
#define WDM_IS_FREE(HDR) (((HDR)->sz & WDM_ALLOC_BIT) == 0)
#define WDM_ACTUAL_SIZE(HDR) ((HDR)->sz & ~WDM_ALLOC_BIT)
#define WDM_IN_HEAP(PTR) (((((uintptr_t)PTR) < (uintptr_t)heap_start + heap_size)) && (((uintptr_t)PTR) >= (uintptr_t)heap_start))

typedef struct wdmalloc_hdr wdmalloc_hdr_t;

// 12 byte header
struct wdmalloc_hdr {
  uint16_t prev_sz;
  uint16_t sz;
  union {
    wdmalloc_hdr_t *next;
    uint64_t magic;
  } lom; // link or magic
} __attribute__((packed));

#ifdef TEST
wdmalloc_hdr_t *freelist_head = NULL;
#else
static wdmalloc_hdr_t *freelist_head = NULL;
#endif

static void *heap_start = NULL;
static uint16_t heap_size = 0;

void wdmalloc_init(void *start_addr, uint16_t size){
  ASSERT(size > sizeof(wdmalloc_hdr_t));

  heap_start = start_addr;
  heap_size = size;

  // Establish initial header
  freelist_head = start_addr;
  freelist_head->prev_sz = 0;
  freelist_head->sz = size - sizeof(wdmalloc_hdr_t);
  freelist_head->lom.next = NULL;
}

void *wdmalloc(uint16_t size){
  if(freelist_head == NULL || heap_start == NULL || heap_size == 0) return NULL;

  wdmalloc_hdr_t *curr = freelist_head;
  wdmalloc_hdr_t *prev = NULL;
  wdmalloc_hdr_t *min = NULL;
  wdmalloc_hdr_t *min_prev = NULL;
  uint16_t min_sz = UINT16_MAX;
  
  // Find best-fit in freelist
  while(curr != NULL){
    if(WDM_IS_FREE(curr) && curr->sz >= size && curr->sz < min_sz){
      min = curr;
      min_prev = prev;
      min_sz = curr->sz;
    }
    prev = curr;
    curr = curr->lom.next;
  }

  // Crash if nothing is found
  if(min == NULL || min_sz == UINT16_MAX) return NULL;

  // Splitting time!
  if(min->sz > size && min->sz - size >= sizeof(wdmalloc_hdr_t)){
    // Size of the block is greater than requested size AND
    // there's enough space for a free header

    // We allocate in the bottom half of the free block, leaving the top half free
    min->sz = (min->sz - size - sizeof(wdmalloc_hdr_t));

    wdmalloc_hdr_t *new_alloc = (wdmalloc_hdr_t *)((uintptr_t)min + sizeof(wdmalloc_hdr_t) + min->sz);
    new_alloc->prev_sz = min->sz;
    new_alloc->sz = size | WDM_ALLOC_BIT;
    new_alloc->lom.magic = WDM_MAGIC;

    wdmalloc_hdr_t *new_alloc_adj = (wdmalloc_hdr_t *)((uintptr_t)new_alloc + sizeof(wdmalloc_hdr_t) + size);
    if(WDM_IN_HEAP(new_alloc_adj)){ // verify not out of bounds
      new_alloc_adj->prev_sz = size;
    }

    return (void*)((uintptr_t)new_alloc + sizeof(wdmalloc_hdr_t));
  } else {
    // Remove min from free list
    if (min_prev == NULL){
      freelist_head = freelist_head->lom.next;
    } else {
      min_prev->lom.next = min->lom.next;
    }

    // Allocate the whole banana
    min->lom.magic = WDM_MAGIC;
    min->sz |= WDM_ALLOC_BIT;

    return (void *)((uintptr_t)min + sizeof(wdmalloc_hdr_t));
  }
}

void *wdcalloc(size_t num, uint16_t size){
  if(freelist_head == NULL || heap_start == NULL || heap_size == 0) return NULL;

  if(num * size > UINT16_MAX) return NULL;

  return wdmalloc(num * size);
}
static void coalesce(wdmalloc_hdr_t *hdr){
  wdmalloc_hdr_t *prev_nghbr = (wdmalloc_hdr_t *)((uintptr_t)hdr - hdr->prev_sz - sizeof(wdmalloc_hdr_t));
  wdmalloc_hdr_t *next_nghbr = (wdmalloc_hdr_t *)((uintptr_t)hdr + sizeof(wdmalloc_hdr_t) + WDM_ACTUAL_SIZE(hdr));
  wdmalloc_hdr_t *next_next_nghbr;

  if(!WDM_IN_HEAP(prev_nghbr)) prev_nghbr = NULL;
  if(!WDM_IN_HEAP(next_nghbr)) next_nghbr = NULL;

  if(next_nghbr) next_next_nghbr = (wdmalloc_hdr_t *)((uintptr_t)next_nghbr + sizeof(wdmalloc_hdr_t) + (next_nghbr->sz & ~WDM_ALLOC_BIT));
  else next_next_nghbr = NULL;

  if(!WDM_IN_HEAP(next_next_nghbr)) next_next_nghbr = NULL;

  if(prev_nghbr && WDM_IS_FREE(prev_nghbr) && next_nghbr && WDM_IS_FREE(next_nghbr)){
    // Merge both
    prev_nghbr->sz += sizeof(wdmalloc_hdr_t) + hdr->sz + sizeof(wdmalloc_hdr_t) + next_nghbr->sz;

    // remove next_nghbr from freelist
    wdmalloc_hdr_t *curr = freelist_head;
    wdmalloc_hdr_t *prev = NULL;
    while(curr != next_nghbr && curr != NULL){
      prev = curr;
      curr = curr->lom.next;
    }

    ASSERT(curr == next_nghbr); // next_nghbr should be found in the freelist; this MUST be true
    
    if(prev) prev->lom.next = next_nghbr->lom.next;
    else freelist_head = next_nghbr->lom.next;

    // Set prev_sz of next_next_nghbr
    if(next_next_nghbr) next_next_nghbr->prev_sz = prev_nghbr->sz;
  } else if(prev_nghbr && WDM_IS_FREE(prev_nghbr)){
    // Merge prev
    prev_nghbr->sz += sizeof(wdmalloc_hdr_t) + hdr->sz;
    
    // Set prev_sz of next_nghbr
    if(next_nghbr){
      next_nghbr->prev_sz = prev_nghbr->sz;
    }
  } else if(next_nghbr && WDM_IS_FREE(next_nghbr)){
    // Merge next
    hdr->sz += sizeof(wdmalloc_hdr_t) + next_nghbr->sz;
    if(next_nghbr->lom.next) hdr->lom.next = next_nghbr->lom.next;
    else hdr->lom.next = NULL;

    // whatever's pointing to next_nghbr here needs to point to hdr now
    // linear search for prev in freelist
    wdmalloc_hdr_t *curr = freelist_head;
    wdmalloc_hdr_t *prev = NULL;
    while(curr != next_nghbr && curr != NULL){
      prev = curr;
      curr = curr->lom.next;
    }

    ASSERT(curr == next_nghbr); // next_nghbr should be found in the freelist; this MUST be true

    // prev is now the element in the freelist before next_nghbr
    if(prev) prev->lom.next = hdr;
    else freelist_head = hdr;

    // Set prev_sz of next_next_nghbr
    if(next_next_nghbr) next_next_nghbr->prev_sz = hdr->sz;
  } else {
    // Merge none, but add to freelist
    wdmalloc_hdr_t *tmp = freelist_head;
    freelist_head = hdr;
    hdr->lom.next = tmp;
  }
}

void *wdrealloc(void* ptr, uint16_t size){
  if(freelist_head == NULL || heap_start == NULL || heap_size == 0) return NULL;

  wdmalloc_hdr_t *hdr = (wdmalloc_hdr_t *)((uintptr_t)ptr - sizeof(wdmalloc_hdr_t));

  if(size == WDM_ACTUAL_SIZE(hdr)){
    return ptr;
  } else if(size > WDM_ACTUAL_SIZE(hdr)){
    wdmalloc_hdr_t *prev_nghbr = (wdmalloc_hdr_t *)((uintptr_t)hdr - hdr->prev_sz - sizeof(wdmalloc_hdr_t));
    wdmalloc_hdr_t *next_nghbr = (wdmalloc_hdr_t *)((uintptr_t)hdr + sizeof(wdmalloc_hdr_t) + WDM_ACTUAL_SIZE(hdr));

    if(!WDM_IN_HEAP(next_nghbr)) next_nghbr = NULL;
    if(!WDM_IN_HEAP(prev_nghbr)) prev_nghbr = NULL;
    
    uint16_t bytes_avail;
    wdmalloc_hdr_t *start;
    wdmalloc_hdr_t *remove[2];

    bool local_reposition = true;
    
    // Scenario 1: combining with next_nghbr gives us enough space
    if(next_nghbr && WDM_IS_FREE(next_nghbr) && (next_nghbr->sz + sizeof(wdmalloc_hdr_t) + WDM_ACTUAL_SIZE(hdr)) >= size){
      bytes_avail = (next_nghbr->sz + sizeof(wdmalloc_hdr_t) + WDM_ACTUAL_SIZE(hdr));
      start = hdr;

      remove[0] = next_nghbr;
      remove[1] = NULL;
    } 
    // Scenario 2: combining with prev_nghbr gives us enough space
    else if(prev_nghbr && WDM_IS_FREE(prev_nghbr) && (prev_nghbr->sz + sizeof(wdmalloc_hdr_t) + WDM_ACTUAL_SIZE(hdr)) >= size){
      bytes_avail = (prev_nghbr->sz + sizeof(wdmalloc_hdr_t) + WDM_ACTUAL_SIZE(hdr));
      start = prev_nghbr;

      remove[0] = prev_nghbr;
      remove[1] = NULL;
    }
    // Scenario 3: we need to combine with both
    else if(next_nghbr && prev_nghbr && WDM_IS_FREE(next_nghbr) && WDM_IS_FREE(prev_nghbr) && 
            (prev_nghbr->sz + sizeof(wdmalloc_hdr_t) + WDM_ACTUAL_SIZE(hdr) + sizeof(wdmalloc_hdr_t) + next_nghbr->sz) >= size){
      bytes_avail = (prev_nghbr->sz + sizeof(wdmalloc_hdr_t) + WDM_ACTUAL_SIZE(hdr) + sizeof(wdmalloc_hdr_t) + next_nghbr->sz);
      start = prev_nghbr;

      remove[0] = prev_nghbr;
      remove[1] = next_nghbr;
    }
    // Scenario 4: we need to find space elsewhere
    else{
      local_reposition = false;
    }

    if(local_reposition){
      // Remove from freelist
      wdmalloc_hdr_t *curr = freelist_head;
      wdmalloc_hdr_t *prev = NULL;
      while(curr != NULL){
        if(curr == remove[0] || curr == remove[1]){
          if(prev) prev->lom.next = curr->lom.next;

          if(curr == freelist_head) freelist_head = curr->lom.next;
          curr = curr->lom.next;
        } else {
          prev = curr;
          curr = curr->lom.next;
        }
      }

      // Check if there's enough space for a split
      if(bytes_avail - size >= sizeof(wdmalloc_hdr_t) + 4){ // at least 4 bytes of free space
        // Split (Alloc | Free)
        start->sz = size | WDM_ALLOC_BIT;
        start->lom.magic = WDM_MAGIC;

        memmove((void *)((uintptr_t)start + sizeof(wdmalloc_hdr_t)), ptr, WDM_ACTUAL_SIZE(hdr));

        wdmalloc_hdr_t *split_loc = (wdmalloc_hdr_t *)((uintptr_t)start + sizeof(wdmalloc_hdr_t) + size);
        split_loc->sz = bytes_avail - size - sizeof(wdmalloc_hdr_t);
        split_loc->prev_sz = size;
        split_loc->lom.next = freelist_head;
        freelist_head = split_loc;

        wdmalloc_hdr_t *after_split = (wdmalloc_hdr_t *)((uintptr_t)split_loc + sizeof(wdmalloc_hdr_t) + split_loc->sz);
        if(WDM_IN_HEAP(after_split)) after_split->prev_sz = split_loc->sz;
      } else {
        start->sz = bytes_avail | WDM_ALLOC_BIT;
        start->lom.magic = WDM_MAGIC;

        wdmalloc_hdr_t *after = (wdmalloc_hdr_t *)((uintptr_t)start + sizeof(wdmalloc_hdr_t) + WDM_ACTUAL_SIZE(start));
        if(WDM_IN_HEAP(after)) after->prev_sz = bytes_avail;

        memmove((void *)((uintptr_t)start + sizeof(wdmalloc_hdr_t)), ptr, WDM_ACTUAL_SIZE(hdr));
      }

      return (void *)((uintptr_t)start + sizeof(wdmalloc_hdr_t));
    } else {
      void *new_loc = wdmalloc(size); // malloc new
      if(!new_loc) return NULL;

      memmove(new_loc, ptr, WDM_ACTUAL_SIZE(hdr)); // copy data

      wdfree(ptr); // free original
      return new_loc;
    }
  } else if(size < WDM_ACTUAL_SIZE(hdr)){
    if(size == 0) {
      wdfree(ptr); 
      return NULL;
    }

    uint16_t bytes_avail = WDM_ACTUAL_SIZE(hdr);

    if(bytes_avail - size >= sizeof(wdmalloc_hdr_t) + 4){
      // Alloc | Free
      hdr->sz = size | WDM_ALLOC_BIT;
      
      wdmalloc_hdr_t *split_loc = (wdmalloc_hdr_t *)((uintptr_t)hdr + sizeof(wdmalloc_hdr_t) + size);
      split_loc->prev_sz = size;
      split_loc->sz = bytes_avail - sizeof(wdmalloc_hdr_t) - size;
      split_loc->lom.next = 0;

      coalesce(split_loc);

      wdmalloc_hdr_t *after_split = (wdmalloc_hdr_t *)((uintptr_t)split_loc + sizeof(wdmalloc_hdr_t) + split_loc->sz);
      if(WDM_IN_HEAP(after_split)) after_split->prev_sz = split_loc->sz;
    }

    return ptr;
  }

  return NULL;
}

void wdfree(void* ptr){
  if(freelist_head == NULL || heap_start == NULL || heap_size == 0) return;

  if(!WDM_IN_HEAP(ptr)) return;

  wdmalloc_hdr_t *hdr = (wdmalloc_hdr_t *)((uintptr_t)ptr - sizeof(wdmalloc_hdr_t));
  if(!WDM_IN_HEAP(hdr)) return;
  
  // Mark as free
  hdr->sz &= ~WDM_ALLOC_BIT;
  hdr->lom.next = 0;

  // Coalesce with neighbors (and add to free list)
  coalesce(hdr);
}

#ifdef TEST
void wd_print_heap_state(){
  wdmalloc_hdr_t *curr = (wdmalloc_hdr_t *)heap_start;
  int block = 0;
  size_t offset = 0;

  printf("=== HEAP STATE (%u bytes total) ===\n", heap_size);

  // Walk all blocks via physical layout (not freelist)
  while(WDM_IN_HEAP(curr)){
    uint16_t raw_sz = curr->sz & ~WDM_ALLOC_BIT;
    int is_free = WDM_IS_FREE(curr);

    printf("[%3d] @%p  off=%-6zu  prev_sz=%-5u  sz=%-5u  %s",
      block,
      (void *)curr,
      offset,
      curr->prev_sz,
      raw_sz,
      is_free ? "FREE" : "USED"
    );

    if(is_free){
      printf("  next=%p", (void *)curr->lom.next);
    } else {
      printf("  magic=%s", curr->lom.magic == WDM_MAGIC ? "OK" : "CORRUPT");
    }

    // Sanity: verify prev_sz matches the actual previous block's sz
    if(offset > 0){
      wdmalloc_hdr_t *prev = (wdmalloc_hdr_t *)((uintptr_t)curr - curr->prev_sz - sizeof(wdmalloc_hdr_t));
      uint16_t prev_raw = prev->sz & ~WDM_ALLOC_BIT;
      if(prev_raw != curr->prev_sz){
        printf("  *** prev_sz MISMATCH (expected %u, got %u) ***", prev_raw, curr->prev_sz);
      }
    }

    printf("\n");

    // Advance to next block
    size_t step = sizeof(wdmalloc_hdr_t) + raw_sz;
    if(step == sizeof(wdmalloc_hdr_t)){
      // zero-size block would loop forever — heap is corrupt
      printf("  *** ZERO-SIZE BLOCK, aborting walk ***\n");
      break;
    }
    offset += step;
    curr = (wdmalloc_hdr_t *)((uintptr_t)curr + step);
    block++;
  }

  // Dump freelist separately to verify linkage
  printf("--- FREELIST ---\n");
  wdmalloc_hdr_t *f = freelist_head;
  int fi = 0;
  while(f != NULL){
    printf("[%3d] @%p  sz=%-5u  next=%p",
      fi, (void*)f, f->sz & ~WDM_ALLOC_BIT, (void*)f->lom.next);
    if(!WDM_IS_FREE(f)){
      printf("  *** ALLOC BIT SET ON FREELIST NODE ***");
    }
    if(!WDM_IN_HEAP(f)){
      printf("  *** OUT OF HEAP ***");
      break; // don't follow a corrupt pointer
    }
    printf("\n");
    f = f->lom.next;
    fi++;
  }
  printf("=== END ===\n");
}
#endif
