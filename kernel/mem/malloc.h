// https://www.gingerbill.org/series/memory-allocation-strategies/
#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

void malloc_init(void *start_addr, size_t size);
void *malloc(size_t size);
void *calloc(size_t num, size_t size);
void *realloc(void* ptr, size_t size);
void *free(void* ptr);

#endif
