#include "wdmalloc.h"
#include "waddletest.h"
#include <stdio.h>
#include <stdint.h>

#define HEAP_SIZE 4096

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

bool test_wdmalloc_basic_alloc_free() {
  uint8_t heap_buf[HEAP_SIZE];
  wdmalloc_init(heap_buf, HEAP_SIZE);

  RUN_AND_PRINT_HEAP_STATE(void *ptr = wdmalloc(3);)
  TEST_ASSERT_LT_PTR ((uint8_t *)ptr, heap_buf + HEAP_SIZE);
  TEST_ASSERT_GEQ_PTR((uint8_t *)ptr, heap_buf);
  memset(ptr, 0xDB, 3);
  TEST_ASSERT_EQ_INT(((uint8_t*)ptr)[2], 0xDB);

  RUN_AND_PRINT_HEAP_STATE(wdfree(ptr);)
  return true;
}

bool test_wdmalloc_null_on_no_init() {
  uint8_t heap_buf[HEAP_SIZE];
  wdmalloc_init(heap_buf, HEAP_SIZE);

  wdmalloc_hdr_t *saved_head = freelist_head;
  freelist_head = NULL;
  void *ptr = wdmalloc(1);
  freelist_head = saved_head;

  TEST_ASSERT_EQ_PTR(ptr, NULL);
  return true;
}

bool test_wdmalloc_null_on_too_large() {
  uint8_t heap_buf[HEAP_SIZE];
  wdmalloc_init(heap_buf, HEAP_SIZE);

  void *ptr = wdmalloc(HEAP_SIZE);
  TEST_ASSERT_EQ_PTR(ptr, NULL);
  return true;
}

bool test_wdmalloc_split_adj_out_of_bounds() {
  uint8_t heap_buf[HEAP_SIZE];
  wdmalloc_init(heap_buf, HEAP_SIZE);

  uint16_t whole_free = freelist_head->sz;
  uint16_t tail_size  = whole_free - sizeof(wdmalloc_hdr_t);

  RUN_AND_PRINT_HEAP_STATE(void *ptr = wdmalloc(tail_size);)
  TEST_ASSERT_LT_PTR ((uint8_t *)ptr, heap_buf + HEAP_SIZE);
  TEST_ASSERT_GEQ_PTR((uint8_t *)ptr, heap_buf);
  memset(ptr, 0xAB, tail_size);
  TEST_ASSERT_EQ_INT(((uint8_t*)ptr)[tail_size - 1], 0xAB);

  RUN_AND_PRINT_HEAP_STATE(wdfree(ptr);)
  return true;
}

bool test_wdmalloc_nosplit_exact_fit_head() {
  uint8_t heap_buf[HEAP_SIZE];
  wdmalloc_init(heap_buf, HEAP_SIZE);

  void *a = wdmalloc(4);
  void *b = wdmalloc(8);
  TEST_ASSERT_NEQ_PTR(a, NULL);
  TEST_ASSERT_NEQ_PTR(b, NULL);

  wdfree(a);
  wdfree(b);

  uint16_t head_sz = WDM_ACTUAL_SIZE(freelist_head);
  RUN_AND_PRINT_HEAP_STATE(void *ptr = wdmalloc(head_sz);)
  TEST_ASSERT_NEQ_PTR(ptr, NULL);
  memset(ptr, 0xCD, head_sz);
  TEST_ASSERT_EQ_INT(((uint8_t*)ptr)[head_sz - 1], 0xCD);

  RUN_AND_PRINT_HEAP_STATE(wdfree(ptr);)
  return true;
}

bool test_wdmalloc_nosplit_exact_fit_mid() {
  uint8_t heap_buf[HEAP_SIZE];
  wdmalloc_init(heap_buf, HEAP_SIZE);

  void *p1 = wdmalloc(4);
  void *p2 = wdmalloc(16);
  void *p3 = wdmalloc(4);
  TEST_ASSERT_NEQ_PTR(p1, NULL);
  TEST_ASSERT_NEQ_PTR(p2, NULL);
  TEST_ASSERT_NEQ_PTR(p3, NULL);

  wdfree(p1);
  wdfree(p2);

  wdmalloc_hdr_t *second = freelist_head->lom.next;
  TEST_ASSERT_NEQ_PTR(second, NULL);
  uint16_t mid_sz = WDM_ACTUAL_SIZE(second);

  RUN_AND_PRINT_HEAP_STATE(void *ptr = wdmalloc(mid_sz);)
  TEST_ASSERT_NEQ_PTR(ptr, NULL);
  memset(ptr, 0xEF, mid_sz);
  TEST_ASSERT_EQ_INT(((uint8_t*)ptr)[mid_sz - 1], 0xEF);

  TEST_ASSERT_NEQ_PTR(freelist_head, NULL);
  TEST_ASSERT_EQ_INT(WDM_IS_FREE(freelist_head), 1);

  wdfree(ptr);
  wdfree(p3);
  return true;
}

// PATH: coalesce – merge none (isolated free block, both neighbors allocated)
bool test_wdmalloc_coalesce_none() {
  uint8_t heap_buf[HEAP_SIZE];
  wdmalloc_init(heap_buf, HEAP_SIZE);

  void *p1 = wdmalloc(8);
  void *p2 = wdmalloc(8);
  void *p3 = wdmalloc(8);
  TEST_ASSERT_NEQ_PTR(p1, NULL);
  TEST_ASSERT_NEQ_PTR(p2, NULL);
  TEST_ASSERT_NEQ_PTR(p3, NULL);

  // p2 is surrounded by allocated blocks → coalesce merges none
  wdfree(p2);

  // freelist head should be p2's block, standalone
  wdmalloc_hdr_t *hdr = (wdmalloc_hdr_t *)((uintptr_t)p2 - sizeof(wdmalloc_hdr_t));
  TEST_ASSERT_EQ_PTR(freelist_head, hdr);
  TEST_ASSERT_EQ_INT(WDM_IS_FREE(freelist_head), 1);
  TEST_ASSERT_EQ_INT(WDM_ACTUAL_SIZE(freelist_head), 8);

  wdfree(p1);
  wdfree(p3);
  return true;
}

// PATH: coalesce – merge next only (prev allocated, next free)
bool test_wdmalloc_coalesce_next() {
  uint8_t heap_buf[HEAP_SIZE];
  wdmalloc_init(heap_buf, HEAP_SIZE);

  void *p1 = wdmalloc(8);
  void *p2 = wdmalloc(8);
  void *p3 = wdmalloc(8);
  TEST_ASSERT_NEQ_PTR(p1, NULL);
  TEST_ASSERT_NEQ_PTR(p2, NULL);
  TEST_ASSERT_NEQ_PTR(p3, NULL);

  wdfree(p3); // free next neighbor first
  wdfree(p2); // freeing p2 should absorb p3

  wdmalloc_hdr_t *hdr_p2 = (wdmalloc_hdr_t *)((uintptr_t)p2 - sizeof(wdmalloc_hdr_t));

  // p2's block should have grown to include p3's header + p3's payload
  TEST_ASSERT_EQ_INT(WDM_ACTUAL_SIZE(hdr_p2), 8 + sizeof(wdmalloc_hdr_t) + 8);
  TEST_ASSERT_EQ_INT(WDM_IS_FREE(hdr_p2), 1);

  wdfree(p1);
  return true;
}

// PATH: coalesce – merge prev only (prev free, next allocated)
bool test_wdmalloc_coalesce_prev() {
  uint8_t heap_buf[HEAP_SIZE];
  wdmalloc_init(heap_buf, HEAP_SIZE);

  void *p1 = wdmalloc(8);
  void *p2 = wdmalloc(8);
  void *p3 = wdmalloc(8);
  TEST_ASSERT_NEQ_PTR(p1, NULL);
  TEST_ASSERT_NEQ_PTR(p2, NULL);
  TEST_ASSERT_NEQ_PTR(p3, NULL);

  wdfree(p1); // free prev neighbor first
  wdfree(p2); // freeing p2 should merge into p1

  wdmalloc_hdr_t *hdr_p1 = (wdmalloc_hdr_t *)((uintptr_t)p1 - sizeof(wdmalloc_hdr_t));

  // p1's block should have grown to include p2's header + p2's payload
  TEST_ASSERT_EQ_INT(WDM_ACTUAL_SIZE(hdr_p1), 8 + sizeof(wdmalloc_hdr_t) + 8);
  TEST_ASSERT_EQ_INT(WDM_IS_FREE(hdr_p1), 1);

  // p3's prev_sz should now reflect the merged block size
  wdmalloc_hdr_t *hdr_p3 = (wdmalloc_hdr_t *)((uintptr_t)p3 - sizeof(wdmalloc_hdr_t));
  TEST_ASSERT_EQ_INT(hdr_p3->prev_sz, WDM_ACTUAL_SIZE(hdr_p1));

  wdfree(p3);
  return true;
}

// PATH: coalesce – merge both (prev free, next free)
bool test_wdmalloc_coalesce_both() {
  uint8_t heap_buf[HEAP_SIZE];
  wdmalloc_init(heap_buf, HEAP_SIZE);

  void *p1 = wdmalloc(8);
  void *p2 = wdmalloc(8);
  void *p3 = wdmalloc(8);
  void *p4 = wdmalloc(8); // fence to prevent p3 merging with remainder
  TEST_ASSERT_NEQ_PTR(p1, NULL);
  TEST_ASSERT_NEQ_PTR(p2, NULL);
  TEST_ASSERT_NEQ_PTR(p3, NULL);
  TEST_ASSERT_NEQ_PTR(p4, NULL);

  wdfree(p1);
  wdfree(p3);
  // now p1=free, p2=alloc, p3=free, p4=alloc
  wdfree(p2); // should merge p1+p2+p3 into one block

  wdmalloc_hdr_t *hdr_p1 = (wdmalloc_hdr_t *)((uintptr_t)p1 - sizeof(wdmalloc_hdr_t));

  uint16_t expected = 8 + sizeof(wdmalloc_hdr_t) + 8 + sizeof(wdmalloc_hdr_t) + 8;
  TEST_ASSERT_EQ_INT(WDM_ACTUAL_SIZE(hdr_p1), expected);
  TEST_ASSERT_EQ_INT(WDM_IS_FREE(hdr_p1), 1);

  // p3's block should no longer be in the freelist (was removed during merge)
  wdmalloc_hdr_t *hdr_p3 = (wdmalloc_hdr_t *)((uintptr_t)p3 - sizeof(wdmalloc_hdr_t));
  wdmalloc_hdr_t *curr = freelist_head;
  while (curr != NULL) {
    TEST_ASSERT_NEQ_PTR(curr, hdr_p3);
    curr = curr->lom.next;
  }

  // p4's prev_sz should reflect the merged block
  wdmalloc_hdr_t *hdr_p4 = (wdmalloc_hdr_t *)((uintptr_t)p4 - sizeof(wdmalloc_hdr_t));
  TEST_ASSERT_EQ_INT(hdr_p4->prev_sz, expected);

  wdfree(p4);
  return true;
}
