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
  uint64_t ofs = bit_index%64;

  bmp_arr[idx] &= ~(1ULL << ofs);
  bmp_arr[idx] |= ((value & 0x1) << ofs);
}

bool bitmap_get(bitmap_t *bmp, uint64_t bit_index){
  ASSERT(bit_index < bmp->size);

  uint64_t *bmp_arr = bmp->buf;
  uint64_t idx = bit_index>>6;
  uint64_t ofs = bit_index%64;

  return (bmp_arr[idx] & (1ULL << ofs)) != 0;
}

void bitmap_flip(bitmap_t *bmp, uint64_t start_idx, size_t num_bits){
  ASSERT(start_idx + num_bits <= bmp->size);

  uint64_t *bmp_arr = bmp->buf;
  uint64_t start_word = (start_idx>>6)+1;
  uint64_t end_word = ((start_idx + num_bits)>>6) + 1;

  uint64_t start_idx_in_word = start_idx%64;
  uint64_t end_idx_in_word = (start_idx + num_bits)%64;
  
  // apply mask for start word
  uint64_t mask = (start_word == end_word)?(((1ULL << num_bits) - 1) << (start_idx_in_word)):(UINT64_MAX << start_idx_in_word);
  bmp_arr[start_word] ^= mask;

  for (uint64_t word_idx = start_word + 1; word_idx < end_word; word_idx++){
    // apply masks for intermediate words
    mask = UINT64_MAX;
    bmp_arr[word_idx] ^= mask;
  }

  // apply mask for end word
  mask = (1ULL << end_idx_in_word) - 1;
  bmp_arr[end_word] ^= mask;
}

uint64_t bitmap_test(bitmap_t *bmp, bool value, uint64_t start_idx, size_t num_bits){
  ASSERT(start_idx + num_bits <= bmp->size);

  uint64_t *bmp_arr = bmp->buf;
  uint64_t end_word = ((start_idx + num_bits)>>6) + 1;

  uint64_t num_bits_curr = num_bits;
  uint64_t candidate_idx = -1ULL;

  for (uint64_t word_idx = start_idx>>6 + 1; word_idx < end_word && num_bits_curr > 0; word_idx++){
    uint64_t val = bmp_arr[word_idx];
    uint64_t norm_val = val ^ (value ? 0ULL : ~0ULL);

    // re-evaluate the mask every iteration because it may have become < 64
    uint64_t mask = (num_bits_curr >= 64) ? UINT64_MAX : ((1ULL << num_bits_curr) - 1); 
    size_t mask_size = (num_bits_curr >= 64) ? 64 : num_bits_curr;

    if (norm_val == 0ULL) continue; // not in this word

    uint64_t idx_in_word = __builtin_ctzll(norm_val); // count trailing zeros to find first 1
    
    // We reset candidate_idx if this is the first bit we're finding
    if(num_bits_curr == num_bits){
      candidate_idx = word_idx * 64 + idx_in_word;
      if (candidate_idx + num_bits >= bmp->size) break; // outside valid range
    }
    
    if (((val >> idx_in_word) & mask) == mask){
      num_bits_curr -= mask_size;
    } else {
      // reset num_bits, the bitset we found did not continue for long enough
      num_bits_curr = num_bits;
    }
  }

  return (num_bits_curr > 0) ? -1ULL : candidate_idx;
}

uint64_t bitmap_test_and_flip(bitmap_t *bmp, bool value){
  uint64_t *bmp_arr = bmp->buf;
  uint64_t words = ROUND_UP_TO(bmp->size, 64);

  for (uint64_t idx = 0; idx < words; idx++) {
    uint64_t val = bmp_arr[idx];
    uint64_t norm_val = val ^ (value ? 0ULL : ~0ULL); // bits to flip become 1

    if (norm_val == 0ULL) continue; // not in this word

    // Find first 1 (i.e. matching bit)
    uint64_t bit = __builtin_ctzll(norm_val); // count trailing zeros to find first 1
    if (idx * 64 + bit >= bmp->size) break; // outside valid range

    bmp_arr[idx] ^= (1ULL << bit);
    return idx * 64 + bit;
  }

  return (uint64_t)-1LL; // not found
}

void bitmap_print(bitmap_t *bmp){
  for(unsigned int i=0; i<bmp->size/8; i++){
    printf(" %x ", ((uint64_t*)bmp->buf)[i]);
  }
}
