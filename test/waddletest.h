#ifndef WADDLETEST_H
#define WADDLETEST_H

#include <stdio.h>
#include <stdbool.h>

#define GREEN "\033[1;32m"
#define RED "\033[1;31m"
#define BLUE "\033[1;34m"
#define MAGENTA  "\033[0;35m"
#define NOCOLOR "\033[0m"

#ifdef TEST_VERBOSE_ASSERTIONS
#define TEST_ASSERT(x) \
    do { \
        if (x) { \
            printf(GREEN "[PASS] " NOCOLOR "%s: %s:%d: ASSERTION %s\n", \
                   __func__, __FILE__, __LINE__, #x); \
        } else { \
            printf(RED "[FAIL] " NOCOLOR "%s: %s:%d: ASSERTION %s\n", \
                   __func__, __FILE__, __LINE__, #x); \
            return false; \
        } \
    } while(0)
#else
#define TEST_ASSERT(x) \
    do { \
        if(!(x)) { \
            printf(RED "[FAIL] " NOCOLOR "%s: %s:%d: ASSERTION %s\n", \
                   __func__, __FILE__, __LINE__, #x); \
            return false; \
        } \
    } while(0)
#endif

#define TEST_ASSERT_EQUAL(x, y) TEST_ASSERT((x) == (y))

typedef struct {
  void (*setup_func)(void);
  bool (*test_func)(void);
  char *test_name;
} test_t;

#endif
