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

static bool regions_overlap(void *a, size_t a_sz, void *b, size_t b_sz) {
  uintptr_t a0 = (uintptr_t)a, a1 = a0 + a_sz;
  uintptr_t b0 = (uintptr_t)b, b1 = b0 + b_sz;
  return a0 < b1 && b0 < a1;
}

bool test_wdmalloc_basic_alloc_free() {
  uint8_t heap[HEAP_SIZE];
  wdmalloc_init(heap, HEAP_SIZE);

  RUN_AND_PRINT_HEAP_STATE(void *p = wdmalloc(64);)
  TEST_ASSERT_NEQ_PTR(p, NULL);
  TEST_ASSERT_GEQ_PTR((uint8_t *)p, heap);
  TEST_ASSERT_LEQ_PTR ((uint8_t *)p + 64, heap + HEAP_SIZE);
  memset(p, 0xAB, 64);
  TEST_ASSERT_EQ_INT(((uint8_t *)p)[63], 0xAB);

  RUN_AND_PRINT_HEAP_STATE(wdfree(p);)
  return true;
}

bool test_wdmalloc_no_overlap() {
  uint8_t heap[HEAP_SIZE];
  wdmalloc_init(heap, HEAP_SIZE);

  RUN_AND_PRINT_HEAP_STATE(void *p1 = wdmalloc(32);)
  RUN_AND_PRINT_HEAP_STATE(void *p2 = wdmalloc(64);)
  RUN_AND_PRINT_HEAP_STATE(void *p3 = wdmalloc(16);)
  TEST_ASSERT_NEQ_PTR(p1, NULL);
  TEST_ASSERT_NEQ_PTR(p2, NULL);
  TEST_ASSERT_NEQ_PTR(p3, NULL);

  TEST_ASSERT_EQ_INT(regions_overlap(p1, 32, p2, 64), false);
  TEST_ASSERT_EQ_INT(regions_overlap(p2, 64, p3, 16), false);
  TEST_ASSERT_EQ_INT(regions_overlap(p1, 32, p3, 16), false);

  RUN_AND_PRINT_HEAP_STATE(wdfree(p1);)
  RUN_AND_PRINT_HEAP_STATE(wdfree(p2);)
  RUN_AND_PRINT_HEAP_STATE(wdfree(p3);)
  return true;
}

bool test_wdmalloc_no_corruption() {
  uint8_t heap[HEAP_SIZE];
  wdmalloc_init(heap, HEAP_SIZE);

  RUN_AND_PRINT_HEAP_STATE(void *p1 = wdmalloc(64);)
  RUN_AND_PRINT_HEAP_STATE(void *p2 = wdmalloc(64);)
  TEST_ASSERT_NEQ_PTR(p1, NULL);
  TEST_ASSERT_NEQ_PTR(p2, NULL);

  memset(p1, 0xAA, 64);
  memset(p2, 0xBB, 64);
  for (int i = 0; i < 64; i++)
    TEST_ASSERT_EQ_INT(((uint8_t *)p1)[i], 0xAA);

  RUN_AND_PRINT_HEAP_STATE(wdfree(p1);)
  RUN_AND_PRINT_HEAP_STATE(wdfree(p2);)
  return true;
}

bool test_wdmalloc_null_on_exhaustion() {
  uint8_t heap[HEAP_SIZE];
  wdmalloc_init(heap, HEAP_SIZE);

  RUN_AND_PRINT_HEAP_STATE(void *p = wdmalloc(HEAP_SIZE);)
  TEST_ASSERT_EQ_PTR(p, NULL);
  return true;
}

bool test_wdmalloc_reuse_after_free() {
  uint8_t heap[HEAP_SIZE];
  wdmalloc_init(heap, HEAP_SIZE);

  RUN_AND_PRINT_HEAP_STATE(void *p = wdmalloc(64);)
  TEST_ASSERT_NEQ_PTR(p, NULL);
  RUN_AND_PRINT_HEAP_STATE(wdfree(p);)

  RUN_AND_PRINT_HEAP_STATE(void *p2 = wdmalloc(64);)
  TEST_ASSERT_NEQ_PTR(p2, NULL);
  memset(p2, 0xCD, 64);
  TEST_ASSERT_EQ_INT(((uint8_t *)p2)[63], 0xCD);

  RUN_AND_PRINT_HEAP_STATE(wdfree(p2);)
  return true;
}

bool test_wdmalloc_coalesce_reclaims_space() {
  uint8_t heap[HEAP_SIZE];
  wdmalloc_init(heap, HEAP_SIZE);

  RUN_AND_PRINT_HEAP_STATE(void *p1 = wdmalloc(128);)
  RUN_AND_PRINT_HEAP_STATE(void *p2 = wdmalloc(128);)
  RUN_AND_PRINT_HEAP_STATE(void *p3 = wdmalloc(128);)
  TEST_ASSERT_NEQ_PTR(p1, NULL);
  TEST_ASSERT_NEQ_PTR(p2, NULL);
  TEST_ASSERT_NEQ_PTR(p3, NULL);

  RUN_AND_PRINT_HEAP_STATE(wdfree(p1);)
  RUN_AND_PRINT_HEAP_STATE(wdfree(p2);)
  RUN_AND_PRINT_HEAP_STATE(wdfree(p3);)

  RUN_AND_PRINT_HEAP_STATE(void *big = wdmalloc(300);)
  TEST_ASSERT_NEQ_PTR(big, NULL);
  memset(big, 0xFF, 300);
  TEST_ASSERT_EQ_INT(((uint8_t *)big)[299], 0xFF);

  RUN_AND_PRINT_HEAP_STATE(wdfree(big);)
  return true;
}

bool test_wdmalloc_full_coalesce() {
  uint8_t heap[HEAP_SIZE];
  wdmalloc_init(heap, HEAP_SIZE);

  #define N 16
  void *ptrs[N];
  for (int i = 0; i < N; i++) {
    RUN_AND_PRINT_HEAP_STATE(ptrs[i] = wdmalloc(32);)
    TEST_ASSERT_NEQ_PTR(ptrs[i], NULL);
  }
  for (int i = 0; i < N; i++) {
    RUN_AND_PRINT_HEAP_STATE(wdfree(ptrs[i]);)
  }

  RUN_AND_PRINT_HEAP_STATE(void *big = wdmalloc(N * 32);)
  TEST_ASSERT_NEQ_PTR(big, NULL);
  RUN_AND_PRINT_HEAP_STATE(wdfree(big);)
  #undef N
  return true;
}

bool test_wdmalloc_interleaved() {
  uint8_t heap[HEAP_SIZE];
  wdmalloc_init(heap, HEAP_SIZE);

  RUN_AND_PRINT_HEAP_STATE(void *a = wdmalloc(32);)
  RUN_AND_PRINT_HEAP_STATE(void *b = wdmalloc(32);)
  TEST_ASSERT_NEQ_PTR(a, NULL);
  TEST_ASSERT_NEQ_PTR(b, NULL);

  RUN_AND_PRINT_HEAP_STATE(wdfree(a);)

  RUN_AND_PRINT_HEAP_STATE(void *c = wdmalloc(32);)
  TEST_ASSERT_NEQ_PTR(c, NULL);
  TEST_ASSERT_EQ_INT(regions_overlap(b, 32, c, 32), false);

  memset(b, 0x11, 32);
  memset(c, 0x22, 32);
  for (int i = 0; i < 32; i++)
    TEST_ASSERT_EQ_INT(((uint8_t *)b)[i], 0x11);

  RUN_AND_PRINT_HEAP_STATE(wdfree(b);)
  RUN_AND_PRINT_HEAP_STATE(wdfree(c);)
  return true;
}

bool test_wdmalloc_zero_size() {
  uint8_t heap[HEAP_SIZE];
  wdmalloc_init(heap, HEAP_SIZE);

  RUN_AND_PRINT_HEAP_STATE(void *p = wdmalloc(0);)
  if (p) { RUN_AND_PRINT_HEAP_STATE(wdfree(p);) }

  RUN_AND_PRINT_HEAP_STATE(void *q = wdmalloc(8);)
  TEST_ASSERT_NEQ_PTR(q, NULL);
  memset(q, 0x55, 8);
  TEST_ASSERT_EQ_INT(((uint8_t *)q)[7], 0x55);
  RUN_AND_PRINT_HEAP_STATE(wdfree(q);)
  return true;
}

bool test_wdrealloc_grows() {
    uint8_t heap[HEAP_SIZE];
    wdmalloc_init(heap, HEAP_SIZE);

    RUN_AND_PRINT_HEAP_STATE(void *p = wdmalloc(32);)
    TEST_ASSERT_NEQ_PTR(p, NULL);
    memset(p, 0xAA, 32);

    RUN_AND_PRINT_HEAP_STATE(void *q = wdrealloc(p, 64);)
    TEST_ASSERT_NEQ_PTR(q, NULL);

    // Old data should be preserved
    for (int i = 0; i < 32; i++)
        TEST_ASSERT_EQ_INT(((uint8_t *)q)[i], 0xAA);

    // New space is writable
    memset((uint8_t *)q + 32, 0xBB, 32);
    TEST_ASSERT_EQ_INT(((uint8_t *)q)[63], 0xBB);

    RUN_AND_PRINT_HEAP_STATE(wdfree(q);)
    return true;
}

bool test_wdrealloc_shrinks() {
    uint8_t heap[HEAP_SIZE];
    wdmalloc_init(heap, HEAP_SIZE);

    RUN_AND_PRINT_HEAP_STATE(void *p = wdmalloc(64);)
    TEST_ASSERT_NEQ_PTR(p, NULL);
    memset(p, 0xCC, 64);

    RUN_AND_PRINT_HEAP_STATE(void *q = wdrealloc(p, 32);)
    TEST_ASSERT_NEQ_PTR(q, NULL);

    for (int i = 0; i < 32; i++)
        TEST_ASSERT_EQ_INT(((uint8_t *)q)[i], 0xCC);

    RUN_AND_PRINT_HEAP_STATE(wdfree(q);)
    return true;
}

bool test_wdrealloc_null_old_pointer() {
    uint8_t heap[HEAP_SIZE];
    wdmalloc_init(heap, HEAP_SIZE);

    // Realloc with NULL should behave like malloc
    RUN_AND_PRINT_HEAP_STATE(void *p = wdrealloc(NULL, 32);)
    TEST_ASSERT_NEQ_PTR(p, NULL);
    memset(p, 0xDD, 32);
    TEST_ASSERT_EQ_INT(((uint8_t *)p)[31], 0xDD);

    RUN_AND_PRINT_HEAP_STATE(wdfree(p);)
    return true;
}

bool test_wdrealloc_zero_size() {
    uint8_t heap[HEAP_SIZE];
    wdmalloc_init(heap, HEAP_SIZE);

    RUN_AND_PRINT_HEAP_STATE(void *p = wdmalloc(32);)
    TEST_ASSERT_NEQ_PTR(p, NULL);
    memset(p, 0xEE, 32);

    // Realloc to zero should free the block and return NULL
    RUN_AND_PRINT_HEAP_STATE(void *q = wdrealloc(p, 0);)
    TEST_ASSERT_EQ_PTR(q, NULL);
    return true;
}

bool test_wdrealloc_exceeds_heap() {
    uint8_t heap[HEAP_SIZE];
    wdmalloc_init(heap, HEAP_SIZE);

    RUN_AND_PRINT_HEAP_STATE(void *p = wdmalloc(64);)
    TEST_ASSERT_NEQ_PTR(p, NULL);

    // Attempt to realloc beyond total heap should fail
    RUN_AND_PRINT_HEAP_STATE(void *q = wdrealloc(p, HEAP_SIZE);)
    TEST_ASSERT_EQ_PTR(q, NULL);

    // Original block should still be valid
    memset(p, 0x11, 64);
    TEST_ASSERT_EQ_INT(((uint8_t *)p)[63], 0x11);

    RUN_AND_PRINT_HEAP_STATE(wdfree(p);)
    return true;
}

bool test_wdrealloc_preserves_non_overlapping_blocks() {
    uint8_t heap[HEAP_SIZE];
    wdmalloc_init(heap, HEAP_SIZE);

    RUN_AND_PRINT_HEAP_STATE(void *a = wdmalloc(32);)
    RUN_AND_PRINT_HEAP_STATE(void *b = wdmalloc(32);)
    TEST_ASSERT_NEQ_PTR(a, NULL);
    TEST_ASSERT_NEQ_PTR(b, NULL);

    // Realloc first block to bigger size
    RUN_AND_PRINT_HEAP_STATE(void *c = wdrealloc(a, 64);)
    TEST_ASSERT_NEQ_PTR(c, NULL);
    TEST_ASSERT_EQ_INT(regions_overlap(c, 64, b, 32), false);

    RUN_AND_PRINT_HEAP_STATE(wdfree(c);)
    RUN_AND_PRINT_HEAP_STATE(wdfree(b);)
    return true;
}
