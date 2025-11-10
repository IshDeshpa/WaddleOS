#include "bitmap.h"
#include "waddletest.h"

#define BMP_SIZE 160
uint8_t buf[20];
bitmap_t bmp;

void setup(){
  // Setup
  for(int i=0; i<sizeof(buf)/sizeof(uint8_t); i++){
    buf[i] = 0xFF;
  }
  
  bitmap_init_buf(&bmp, buf, BMP_SIZE);
}

bool waddletest_bitmap_init_buf(){
  // None of the bits should have changed
  for(int i=0; i<sizeof(buf)/sizeof(uint8_t); i++){
    TEST_ASSERT_EQUAL(buf[i], 0xFF);
  }

  // Buf should be buf
  TEST_ASSERT_EQUAL(bmp.buf, buf);

  // Size should be correct
  TEST_ASSERT_EQUAL(bmp.size, BMP_SIZE);

  return true;
}

bool waddletest_bitmap_set(){
  for(int ind=0; ind<BMP_SIZE; ind++){
    // Bounds check
    int ind = 0;
    int bitind = ind%64;
    int wordind = ind/64;
    uint8_t prev = buf[wordind];

    bitmap_set(&bmp, ind, false);
    TEST_ASSERT_EQUAL(buf[wordind], prev & ~(1 << bitind));
  }

  return true;
}

bool waddletest_bitmap_get(){
  // Alternating bits
  for(int i=0; i<sizeof(buf)/sizeof(uint8_t); i++){
    buf[i] = 0xAA;
  }

  for(int ind=0; ind<BMP_SIZE; ind++){
    TEST_ASSERT_EQUAL(bitmap_get(&bmp, ind), (bool)(ind%2));
  }

  return true;
}


