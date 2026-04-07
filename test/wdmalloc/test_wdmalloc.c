#include "wdmalloc.h"
#include "waddletest.h"
#include <stdio.h>

#define HEAP_SIZE 4096
uint8_t heap_buf[HEAP_SIZE];

void setup_heap(){
  memset(heap_buf, 0xFF, HEAP_SIZE);
  wdmalloc_init(heap_buf, HEAP_SIZE);
}

void wd_print_heap_state();
bool test_heap_1(){
  void *ptr = wdmalloc(3); // allocate three bytes

  printf("\nptr = wdmalloc(3);\n\r");
  wd_print_heap_state();
  
  // bounds check
  TEST_ASSERT_LT_PTR((uint8_t*)ptr, heap_buf+HEAP_SIZE);
  TEST_ASSERT_GEQ_PTR((uint8_t*)ptr, heap_buf);

  // pointer should be usable
  memset(ptr, 0xAB, 3);
  TEST_ASSERT_EQ_INT(((uint8_t*)ptr)[2], 0xAB);

  wdfree(ptr);

  printf("\nwdfree(ptr);\n\r");
  wd_print_heap_state();

  // after free, the same address should be reused on next alloc of same size
  void *ptr2 = wdmalloc(3);

  printf("\nptr2 = wdmalloc(3);\n\r");
  wd_print_heap_state();

  TEST_ASSERT_EQ_PTR(ptr2, ptr);

  wdfree(ptr2);

  printf("\nwdfree(ptr2);\n\r");
  wd_print_heap_state();

  // alloc after free should still be in bounds
  void *ptr3 = wdmalloc(3);

  printf("\nptr3 = wdmalloc(3);\n\r");
  wd_print_heap_state();

  TEST_ASSERT_LT_PTR((uint8_t*)ptr3, heap_buf+HEAP_SIZE);
  TEST_ASSERT_GEQ_PTR((uint8_t*)ptr3, heap_buf);
  wdfree(ptr3);

  printf("\nwdfree(ptr3);\n\r");
  wd_print_heap_state();

  // double alloc: freeing first should not corrupt second
  void *a = wdmalloc(3);

  printf("\na = wdmalloc(3);\n\r");
  wd_print_heap_state();

  void *b = wdmalloc(3);

  printf("\nb = wdmalloc(3);\n\r");
  wd_print_heap_state();

  TEST_ASSERT_NEQ_PTR(a, b);
  memset(b, 0xCD, 3);

  wdfree(a);
  
  printf("\nwdfree(a);\n\r");
  wd_print_heap_state();

  TEST_ASSERT_EQ_INT(((uint8_t*)b)[0], 0xCD);  // b's contents should be intact
  TEST_ASSERT_EQ_INT(((uint8_t*)b)[2], 0xCD);

  wdfree(b);

  printf("\nwdfree(b);\n\r");
  wd_print_heap_state();

  return true;
}
