// https://www.gingerbill.org/series/memory-allocation-strategies/
#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

void *kernel_malloc(size_t size);
void *kernel_calloc(size_t num, size_t size);
void *kernel_realloc(void* ptr, size_t size);
void *kernel_free(void* ptr);

#endif
