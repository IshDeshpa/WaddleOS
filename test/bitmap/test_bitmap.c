#include "bitmap.h"
#include "waddletest.h"

#define BMP_SIZE ((long)160)
uint8_t buf[20];
bitmap_t bmp;

void setup_bitmap(){
  // Setup
  for(int i=0; i<(int)(sizeof(buf)/sizeof(uint8_t)); i++){
    buf[i] = 0xFF;
  }
  
  bitmap_init_buf(&bmp, buf, BMP_SIZE);
}

bool test_bitmap_init_buf(){
  // bitmap_init_buf initializes to 0
  for(int i=0; i<(int)(sizeof(buf)/sizeof(uint8_t)); i++){
    TEST_ASSERT_EQUAL_INT(buf[i], 0x0);
  }

  // Buf should be buf
  TEST_ASSERT_EQUAL_PTR(bmp.buf, buf);

  // Size should be correct
  TEST_ASSERT_EQUAL_SIZE(bmp.size, BMP_SIZE);

  return true;
}

bool test_bitmap_set(){
  for(int ind=0; ind<BMP_SIZE; ind++){
    // Bounds check
    int ind = 0;
    int bitind = ind%64;
    int wordind = ind/64;
    uint8_t prev = buf[wordind];

    bitmap_set(&bmp, ind, false);
    TEST_ASSERT_EQUAL_INT(buf[wordind], prev & ~(1 << bitind));
  }

  return true;
}

bool test_bitmap_get(){
  // Alternating bits
  for(int i=0; i<(int)(sizeof(buf)/sizeof(uint8_t)); i++){
    buf[i] = 0xAA;
  }

  for(int ind=0; ind<BMP_SIZE; ind++){
    TEST_ASSERT_EQUAL_INT(bitmap_get(&bmp, ind), (bool)(ind%2));
  }

  return true;
}

bool test_bitmap_flip(){
  for (int i = 0; i < (int)(sizeof(buf) / sizeof(uint8_t)); i++) {
    buf[i] = 0xAA;
  }

  // Flip entire bitmap
  bitmap_flip(&bmp, 0, BMP_SIZE-1);
  for (int i = 0; i < (int)(sizeof(buf) / sizeof(uint8_t)); i++) {
    TEST_ASSERT_EQUAL_INT(buf[i], (uint8_t)0x55);
  }

  // Flip entire bitmap (again)
  bitmap_flip(&bmp, 0, BMP_SIZE-1);
  for (int i = 0; i < (int)(sizeof(buf) / sizeof(uint8_t)); i++) {
    TEST_ASSERT_EQUAL_INT(buf[i], (uint8_t)0xAA);
  }

  // Flip 8 bits starting at bit 0
  bitmap_flip(&bmp, 0, 7);

  // Expect first byte to have flipped bits: 0b01010101 = 0x55
  TEST_ASSERT_EQUAL_INT(buf[0], 0x55);

  // Other bytes should remain unchanged
  for (int i = 1; i < (int)(sizeof(buf) / sizeof(uint8_t)); i++) {
    TEST_ASSERT_EQUAL_INT(buf[i], 0xAA);
  }

  // Flip bits 4–11 (crosses byte boundary)
  bitmap_flip(&bmp, 4, 11);
  // Byte 0: bits 4–7 flipped → 0x55 -> bits 4–7 flip -> 0xA5
  TEST_ASSERT_EQUAL_INT(buf[0], 0xA5);
  // Byte 1: bits 0–3 flipped from 0xAA -> 0xA5
  TEST_ASSERT_EQUAL_INT(buf[1], 0xA5);

  return true;
}

bool test_bitmap_test(){
  // Find a valid run of 8 1s
  int64_t ret;

  ret = bitmap_test(&bmp, true, 0, BMP_SIZE-1, 8);
  TEST_ASSERT_EQUAL_INT(ret, -1); // no run of 1s exists 

  ret = bitmap_test(&bmp, false, 0, BMP_SIZE-1, 8);
  TEST_ASSERT_EQUAL_INT(ret, 0); // first run of 8 0s is at index 0 (since everything is initialized to 0)
  
  buf[2] = 0xA3;

  ret = bitmap_test(&bmp, true, 0, BMP_SIZE-1, 2);
  TEST_ASSERT_EQUAL_INT(ret, 16);

  return true;
}
