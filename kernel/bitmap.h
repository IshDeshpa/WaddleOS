#ifndef BITMAP
#define BITMAP

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void bitmap_init(void *bitmap_buffer, size_t len_bytes);
void bitmap_set(void *bitmap_buffer, uint64_t bit, bool val);
bool bitmap_get(void *bitmap_buffer, uint64_t bit);

#endif
