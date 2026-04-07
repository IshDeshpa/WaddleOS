#ifndef WADDLETEST_H
#define WADDLETEST_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

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

#define TEST_ASSERT_CMP(x, y, cmp, cmp_str, format) \
  do { \
    if (!((x) cmp (y))) { \
      printf(RED "[FAIL] " NOCOLOR "%s: %s:%d: ASSERTION (%s %s %s) failed: ", \
             __func__, __FILE__, __LINE__, #x, cmp_str, #y); \
      printf("!(" format cmp_str format ")\n", (x), (y)); \
      return false; \
    } else if (TEST_VERBOSE) { \
      printf(GREEN "[PASS] " NOCOLOR "%s: %s:%d: ASSERTION (%s %s %s) passed: ", \
             __func__, __FILE__, __LINE__, #x, cmp_str, #y); \
      printf("!(" format cmp_str format ")\n", (x), (y)); \
    } \
  } while (0)

#define TEST_ASSERT_EQ(x, y, fmt) TEST_ASSERT_CMP(x, y, ==, "==", fmt)
#define TEST_ASSERT_NEQ(x, y, fmt) TEST_ASSERT_CMP(x, y, !=, "!=", fmt)
#define TEST_ASSERT_LT(x, y, fmt)    TEST_ASSERT_CMP(x, y, <,  "<",  fmt)
#define TEST_ASSERT_GT(x, y, fmt)    TEST_ASSERT_CMP(x, y, >,  ">",  fmt)
#define TEST_ASSERT_LEQ(x, y, fmt)   TEST_ASSERT_CMP(x, y, <=,  "<=",  fmt)
#define TEST_ASSERT_GEQ(x, y, fmt)   TEST_ASSERT_CMP(x, y, >=,  ">=",  fmt)

#define TEST_ASSERT_EQ_INT(x, y) TEST_ASSERT_EQ(x, y, "%d")
#define TEST_ASSERT_NEQ_INT(x, y) TEST_ASSERT_NEQ(x, y, "%d")
#define TEST_ASSERT_LT_INT(x, y) TEST_ASSERT_LT(x, y, "%d")
#define TEST_ASSERT_GT_INT(x, y) TEST_ASSERT_GT(x, y, "%d")
#define TEST_ASSERT_LEQ_INT(x, y) TEST_ASSERT_LEQ(x, y, "%d")
#define TEST_ASSERT_GEQ_INT(x, y) TEST_ASSERT_GEQ(x, y, "%d")

#define TEST_ASSERT_EQ_LONG(x, y) TEST_ASSERT_EQ((long)x, (long)y, "%ld")
#define TEST_ASSERT_NEQ_LONG(x, y) TEST_ASSERT_NEQ((long)x, (long)y, "%ld")
#define TEST_ASSERT_LT_LONG(x, y) TEST_ASSERT_LT((long)x, (long)y, "%ld")
#define TEST_ASSERT_GT_LONG(x, y) TEST_ASSERT_GT((long)x, (long)y, "%ld")
#define TEST_ASSERT_LEQ_LONG(x, y) TEST_ASSERT_LEQ((long)x, (long)y, "%ld")
#define TEST_ASSERT_GEQ_LONG(x, y) TEST_ASSERT_GEQ((long)x, (long)y, "%ld")

#define TEST_ASSERT_EQ_PTR(x, y) TEST_ASSERT_EQ(x, y, "%p")
#define TEST_ASSERT_NEQ_PTR(x, y) TEST_ASSERT_NEQ(x, y, "%p")
#define TEST_ASSERT_LT_PTR(x, y) TEST_ASSERT_LT(x, y, "%p")
#define TEST_ASSERT_GT_PTR(x, y) TEST_ASSERT_GT(x, y, "%p")
#define TEST_ASSERT_LEQ_PTR(x, y) TEST_ASSERT_LEQ(x, y, "%p")
#define TEST_ASSERT_GEQ_PTR(x, y) TEST_ASSERT_GEQ(x, y, "%p")

typedef struct {
  void (*setup_func)(void);
  bool (*test_func)(void);
  char *test_name;
} test_t;

#endif
