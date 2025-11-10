#ifndef WADDLETEST_H
#define WADDLETEST_H
#include <stdio.h>

// Assume a list of test functions has been provided
#define TEST_ASSERT(x) do{ printf("%s: ASSERTION %s:%d %s\n\r", __func__, __FILE__, __LINE__, (x)?"PASSED":"FAILED"); if(!(x)) return false;} while(0)
#define TEST_ASSERT_EQUAL(x, y) TEST_ASSERT((x) == (y))
#endif
