#ifndef WADDLETEST_H
#define WADDLETEST_H

#include <stdio.h>
#include <stdbool.h>

#define GREEN "\033[1;32m"
#define RED "\033[1;31m"
#define BLUE "\033[1;34m"
#define MAGENTA  "\033[0;35m"
#define NOCOLOR "\033[0m"

#ifdef TEST_VERBOSE
#define TEST_VERBOSE 1
#else
#define TEST_VERBOSE 0
#endif

#define TEST_ASSERT_EQUAL(x, y, format) \
  do { \
    if ((x) != (y)) { \
      printf(RED "[FAIL] " NOCOLOR "%s: %s:%d: ASSERTION (%s == %s) failed: ", \
             __func__, __FILE__, __LINE__, #x, #y); \
      printf("got " format ", expected " format "\n", x, y); \
      return false; \
    } else { \
      if(TEST_VERBOSE) \
        printf(GREEN "[PASS] " NOCOLOR "%s: (" format " == " format ")\n", __func__, x, y); \
    } \
  } while(0)

#define TEST_ASSERT_EQUAL_INT(x, y) TEST_ASSERT_EQUAL(x, y, "%d")
#define TEST_ASSERT_EQUAL_SIZE(x, y) TEST_ASSERT_EQUAL(x, y, "%lu")
#define TEST_ASSERT_EQUAL_PTR(x, y) TEST_ASSERT_EQUAL(x, y, "%p")

typedef struct {
  void (*setup_func)(void);
  bool (*test_func)(void);
  char *test_name;
} test_t;

#endif
