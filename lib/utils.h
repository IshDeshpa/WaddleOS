#ifndef UTILS_H
#define UTILS_H

#ifndef TEST
#define STATIC static inline
#else
#include "waddletest.h"
#define STATIC
#endif

#define ASSERT(x) do{while(!(x)){  __asm__ volatile("hlt"); }}while(0)
#define ROUND_UP_TO(x, r) ((x + r) & ~r)
#define MAX(x, y) ((x > y)?x:y)
#define MIN(x, y) ((x < y)?x:y)

#endif
