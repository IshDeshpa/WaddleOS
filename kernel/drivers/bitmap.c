#include "bitmap.h"
#include "utils.h"
#include "printf.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

void bitmap_init_buf(bitmap_t *bmp, void *buf, size_t num_idx){
  ASSERT(buf != 0);

  bmp->buf = buf;
  bmp->size = num_idx;

  memset(buf, 0, ROUND_UP_TO(num_idx, 8));
}

void bitmap_set(bitmap_t *bmp, uint64_t bit_index, bool value){
  ASSERT(bit_index < bmp->size);

  uint64_t *bmp_arr = bmp->buf;
  uint64_t idx = bit_index>>6;
  uint64_t ofs = bit_index & 63;

  bmp_arr[idx] &= ~(1ULL << ofs);
  bmp_arr[idx] |= ((value & 0x1) << ofs);
}

bool bitmap_get(bitmap_t *bmp, uint64_t bit_index){
  ASSERT(bit_index < bmp->size);

  uint64_t *bmp_arr = bmp->buf;
  uint64_t idx = bit_index>>6;
  uint8_t ofs = bit_index & 63;

  return (bmp_arr[idx] & (1ULL << ofs)) != 0;
}

// Flip all bits from start_idx to end_idx (inclusive)
void bitmap_flip(bitmap_t *bmp, uint64_t start_idx, uint64_t end_idx){
  ASSERT(start_idx <= bmp->size);
  ASSERT(end_idx <= bmp->size);
  ASSERT(start_idx <= end_idx);

  uint64_t *bmp_arr = bmp->buf;

  uint64_t start_word = (start_idx>>6); // / 64
  uint64_t end_word = (end_idx>>6); // / 64

  uint8_t start_idx_in_word = start_idx&63; // % 64
  uint8_t end_idx_in_word = end_idx&63; // % 64
  
  // apply mask for start word
  uint64_t mask;
  if (start_word == end_word){
    uint8_t num_bits = end_idx - start_idx + 1;
    
    if(num_bits == 64) mask = UINT64_MAX;
    else mask = ((1ULL << num_bits) - 1) << (start_idx_in_word);

    bmp_arr[start_word] ^= mask;
    return;
  }

  bmp_arr[start_word] ^= UINT64_MAX << start_idx_in_word;

  for (uint64_t word_idx = start_word+1; word_idx < end_word; word_idx++){
    // apply masks for intermediate words
    mask = UINT64_MAX;
    bmp_arr[word_idx] ^= mask;
  }

  // apply mask for end word
  mask = (end_idx_in_word == 63)?UINT64_MAX:((1ULL << (end_idx_in_word+1)) - 1);
  bmp_arr[end_word] ^= mask;
}

// Test for num_bits consecutive values of value in between start_idx and end_idx
// Returns the first bit in the series if found, or -1 if not
int64_t bitmap_test(bitmap_t *bmp, bool value, uint64_t start_idx, uint64_t end_idx, size_t num_bits){
  ASSERT(start_idx < bmp->size);
  ASSERT(end_idx < bmp->size);
  ASSERT(start_idx <= end_idx);
  ASSERT(start_idx + num_bits - 1 <= end_idx);
  ASSERT(num_bits > 0);

  size_t num_bits_iter = 0;
  uint64_t candidate_idx = start_idx;
  for (uint64_t i=start_idx; i<=end_idx && num_bits_iter < num_bits; i++) {
    if(bitmap_get(bmp, i) == value){
      if (num_bits_iter == 0) candidate_idx = i; 

      num_bits_iter++;
    } else {
      num_bits_iter = 0;
    }
  }

  return (num_bits_iter == num_bits)?candidate_idx:-1;
}

uint64_t bitmap_test_and_flip(bitmap_t *bmp, bool value, uint64_t start_idx, uint64_t end_idx, size_t num_bits){
  uint64_t *bmp_arr = bmp->buf;
}

void bitmap_print(bitmap_t *bmp){
  for(unsigned int i=0; i<bmp->size/8; i++){
    printf(" %lx ", ((uint64_t*)bmp->buf)[i]);
  }
}
