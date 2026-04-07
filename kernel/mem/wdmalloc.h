// https://www.gingerbill.org/series/memory-allocation-strategies/
#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>

void wdmalloc_init(void *start_addr, uint16_t size);
void *wdmalloc(uint16_t size);
void *wdcalloc(size_t num, uint16_t size);
void *wdrealloc(void* ptr, uint16_t size);
void wdfree(void* ptr);

#endif
