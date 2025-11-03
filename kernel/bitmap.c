#include "bitmap.h"
#include "utils.h"
#include "printf.h"
#include <string.h>
#include <stdbool.h>

void bitmap_init_buf(bitmap_t *bmp, void *buf, size_t num_idx){
  ASSERT(buf != 0);

  bmp->buf = buf;
  bmp->size = num_idx;

  memset(buf, 0, (num_idx+7)/8);
}

void bitmap_set(bitmap_t *bmp, uint64_t bit_index, bool value){
  ASSERT(bit_index < bmp->size);

  uint64_t *bmp_arr = bmp->buf;
  uint64_t idx = bit_index/64;
  uint64_t ofs = bit_index%64;

  bmp_arr[idx] &= ~(1ULL << ofs);
  bmp_arr[idx] |= ((value & 0x1) << ofs);
}

bool bitmap_get(bitmap_t *bmp, uint64_t bit_index){
  ASSERT(bit_index < bmp->size);

  uint64_t *bmp_arr = bmp->buf;
  uint64_t idx = bit_index/64;
  uint64_t ofs = bit_index%64;

  return (bmp_arr[idx] & (1ULL << ofs)) != 0;
}

void bitmap_flip(bitmap_t *bmp, uint64_t bit_index){
  ASSERT(bit_index < bmp->size);

  uint64_t *bmp_arr = bmp->buf;
  uint64_t idx = bit_index/64;
  uint64_t ofs = bit_index%64;

  bmp_arr[idx] ^= (1ULL << ofs);
}

uint64_t bitmap_test_and_flip(bitmap_t *bmp, bool value_to_test) {
  uint64_t *bmp_arr = bmp->buf;
  uint64_t words = (bmp->size + 63) / 64;

  for (uint64_t idx = 0; idx < words; idx++) {
    uint64_t val = bmp_arr[idx];
    uint64_t norm_val = val ^ (value_to_test ? 0ULL : ~0ULL); // bits to test become 0

    // Find first zero (i.e. matching bit)
    if (norm_val != 0ULL) {
      uint64_t bit = __builtin_ctzll(norm_val); // count trailing zeros 
      if (idx * 64 + bit >= bmp->size)
        break; // outside valid range
      bmp_arr[idx] ^= (1ULL << bit);
      return idx * 64 + bit;
    }
  }

  return (uint64_t)-1LL; // not found
}

void bitmap_print(bitmap_t *bmp){
  for(unsigned int i=0; i<bmp->size/8; i++){
    printf(DEV_SERIAL_COM1, " %x ", ((uint64_t*)bmp->buf)[i]);
  }
}
