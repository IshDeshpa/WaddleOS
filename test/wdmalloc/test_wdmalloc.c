#include "wdmalloc.h"
#include "waddletest.h"
#include <stdio.h>
#include <stdint.h>

#define HEAP_SIZE 4096
uint8_t heap_buf[HEAP_SIZE];

void wd_print_heap_state();

#if (TEST_VERBOSE == 1)
#define RUN_AND_PRINT_HEAP_STATE(call) \
  call \
  printf("%s\n", #call); \
  wd_print_heap_state();
#else
#define RUN_AND_PRINT_HEAP_STATE(call) \
  call
#endif

void setup_heap(){
  memset(heap_buf, 0xFF, HEAP_SIZE);
}

// COPIED FROM WDMALLOC
typedef struct wdmalloc_hdr wdmalloc_hdr_t;
extern wdmalloc_hdr_t *freelist_head;

// 12 byte header
struct wdmalloc_hdr {
  uint16_t prev_sz;
  uint16_t sz;
  union {
    wdmalloc_hdr_t *next;
    uint64_t magic;
  } lom; // link or magic
} __attribute__((packed));

#define WDM_ALLOC_BIT (1 << 15)
#define WDM_MAGIC (0xDEADBEEFBABECAFE)
#define WDM_IS_FREE(HDR) (((HDR)->sz & WDM_ALLOC_BIT) == 0)
#define WDM_ACTUAL_SIZE(HDR) ((HDR)->sz & ~WDM_ALLOC_BIT)
#define WDM_IN_HEAP(PTR) (((((uintptr_t)PTR) < (uintptr_t)heap_start + heap_size)) && (((uintptr_t)PTR) >= (uintptr_t)heap_start))
// -----------------------

bool test_wdmalloc_1(){
  wdmalloc_init(heap_buf, HEAP_SIZE);

  // ── Already tested: split with adj block inside heap ──────────────────────
  RUN_AND_PRINT_HEAP_STATE(
    void *ptr = wdmalloc(3);
  )
  TEST_ASSERT_LT_PTR ((uint8_t *)ptr, heap_buf + HEAP_SIZE);
  TEST_ASSERT_GEQ_PTR((uint8_t *)ptr, heap_buf);
  memset(ptr, 0xDB, 3);
  TEST_ASSERT_EQ_INT(((uint8_t*)ptr)[2], 0xDB);

  RUN_AND_PRINT_HEAP_STATE(
    wdfree(ptr);
  )

  // ── PATH: NULL – heap not initialised ────────────────────────────────────
  // Temporarily break state by saving/restoring freelist_head.
  wdmalloc_hdr_t *saved_head = freelist_head;
  freelist_head = NULL;
  void *null_ptr = wdmalloc(1);
  TEST_ASSERT_EQ_PTR(null_ptr, NULL);
  freelist_head = saved_head;

  // ── PATH: NULL – no block large enough ───────────────────────────────────
  null_ptr = wdmalloc(HEAP_SIZE); // larger than any single free block
  TEST_ASSERT_EQ_PTR(null_ptr, NULL);

  // ── PATH: split where new_alloc_adj lands OUT OF BOUNDS ──────────────────
  // Allocate a block whose user data ends exactly at heap boundary so that
  // the would-be adjacent header is outside the heap.
  // Figure out how many bytes of user data that requires:
  //   heap_buf + HEAP_SIZE
  //     == (freelist_head) + sizeof(hdr) + free_remaining
  //     == new_alloc      + sizeof(hdr) + req_size
  // So req_size = free_remaining - sizeof(hdr).
  // After the reinit the whole heap is one free block:
  wdmalloc_init(heap_buf, HEAP_SIZE);
  uint16_t whole_free  = freelist_head->sz;          // full usable payload
  uint16_t tail_size   = whole_free - sizeof(wdmalloc_hdr_t); // leaves hdr room for split
  RUN_AND_PRINT_HEAP_STATE(
    void *tail_ptr = wdmalloc(tail_size);
  )
  TEST_ASSERT_LT_PTR ((uint8_t *)tail_ptr, heap_buf + HEAP_SIZE);
  TEST_ASSERT_GEQ_PTR((uint8_t *)tail_ptr, heap_buf);
  // The adj-block check (WDM_IN_HEAP) must have been false – no crash = pass.
  memset(tail_ptr, 0xAB, tail_size);
  TEST_ASSERT_EQ_INT(((uint8_t*)tail_ptr)[tail_size - 1], 0xAB);

  RUN_AND_PRINT_HEAP_STATE(
    wdfree(tail_ptr);
  )

  // ── PATH: no-split, min_prev == NULL (exact fit on freelist head) ─────────
  // Re-init so we have one big free block, then fragment it:
  //   alloc A (small) → alloc B (exact target size) → free A, free B
  // Now freelist has two nodes; request exact size of B's block so the
  // best-fit winner is B (head of list after re-ordering) with prev == NULL.
  wdmalloc_init(heap_buf, HEAP_SIZE);
  void *a = wdmalloc(4);   // carve out block A
  void *b = wdmalloc(8);   // carve out block B – this is our exact-fit target
  TEST_ASSERT_NEQ_PTR(a, NULL);
  TEST_ASSERT_NEQ_PTR(b, NULL);

  wdfree(a);                // return A to freelist first → it becomes head
  wdfree(b);                // return B; list now: A → B (or coalesced – impl-dependent)

  // Determine the exact payload size of the block now at freelist_head:
  uint16_t head_sz = WDM_ACTUAL_SIZE(freelist_head); // strip alloc bit just in case
  RUN_AND_PRINT_HEAP_STATE(
    void *exact_head = wdmalloc(head_sz);   // must consume head, no split
  )
  TEST_ASSERT_NEQ_PTR(exact_head, NULL);
  memset(exact_head, 0xCD, head_sz);
  TEST_ASSERT_EQ_INT(((uint8_t*)exact_head)[head_sz - 1], 0xCD);

  RUN_AND_PRINT_HEAP_STATE(
    wdfree(exact_head);
  )

  // ── PATH: no-split, min_prev != NULL (exact fit on a non-head node) ───────
  wdmalloc_init(heap_buf, HEAP_SIZE);

  // Carve three blocks so we get a non-head free node to target:
  void *p1 = wdmalloc(4);    // block 1 (will stay allocated → skipped)
  void *p2 = wdmalloc(16);   // block 2 (will be freed → becomes a mid node)
  void *p3 = wdmalloc(4);    // block 3 (will stay allocated → acts as fence)
  TEST_ASSERT_NEQ_PTR(p1, NULL);
  TEST_ASSERT_NEQ_PTR(p2, NULL);
  TEST_ASSERT_NEQ_PTR(p3, NULL);

  // Free only p1 and p2 so freelist = [p1_block] → [p2_block] → [remainder].
  // p2_block is a non-head node.
  wdfree(p1);
  wdfree(p2);

  // Walk to the second free node to get its exact size:
  wdmalloc_hdr_t *second = freelist_head->lom.next;
  TEST_ASSERT_NEQ_PTR(second, NULL);
  uint16_t mid_sz = WDM_ACTUAL_SIZE(second);

  // Requesting mid_sz with no room to split should consume that node exactly,
  // exercising the min_prev != NULL branch.
  RUN_AND_PRINT_HEAP_STATE(
    void *exact_mid = wdmalloc(mid_sz);
  )
  TEST_ASSERT_NEQ_PTR(exact_mid, NULL);
  memset(exact_mid, 0xEF, mid_sz);
  TEST_ASSERT_EQ_INT(((uint8_t*)exact_mid)[mid_sz - 1], 0xEF);

  // Verify freelist head is still intact (p1's block), not corrupted.
  TEST_ASSERT_NEQ_PTR(freelist_head, NULL);
  TEST_ASSERT_EQ_INT(WDM_IS_FREE(freelist_head), 1);

  wdfree(exact_mid);
  wdfree(p3);

  return true;
}
