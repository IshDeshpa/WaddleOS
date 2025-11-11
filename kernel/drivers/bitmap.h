#ifndef BITMAP
#define BITMAP

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
  void *buf;
  size_t size;
} bitmap_t;

void bitmap_init_buf(bitmap_t *bmp, void *buf, size_t num_idx);
void bitmap_set(bitmap_t *bmp, uint64_t bit_index, bool value);
bool bitmap_get(bitmap_t *bmp, uint64_t bit_index);
void bitmap_flip(bitmap_t *bmp, uint64_t start_idx, size_t num_bits);

#endif
