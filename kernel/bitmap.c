#include "bitmap.h"
#include "utils.h"
#include <string.h>

#define BITMAP_SIZE(buffer) (((uint16_t*)buffer)[0])
#define BITMAP_CONTENTS(buffer) (&((uint8_t*)buffer)[2])

void bitmap_init(void *bitmap_buffer, size_t len_bytes){
  // Rule: first 2 bytes hold size of bitmap
  BITMAP_SIZE(bitmap_buffer) = (uint16_t)len_bytes;

  // Clear bitmap
  memset(BITMAP_CONTENTS(bitmap_buffer), 0, len_bytes);
}

void bitmap_set(void *bitmap_buffer, uint64_t bit, bool val){
  ASSERT(BITMAP_SIZE(bitmap_buffer) > bit/8);
 
  uint8_t byte_bitpos = bit%8;
  BITMAP_CONTENTS(bitmap_buffer)[bit/8] = (BITMAP_CONTENTS(bitmap_buffer)[bit/8] & ~(1 << byte_bitpos)) | ((val?1:0) << byte_bitpos);
}

bool bitmap_get(void *bitmap_buffer, uint64_t bit){
  ASSERT(BITMAP_SIZE(bitmap_buffer) > bit/8);

  uint8_t byte_bitpos = bit%8;
  return (BITMAP_CONTENTS(bitmap_buffer)[bit/8] & (1 << byte_bitpos)) == 1;
}
