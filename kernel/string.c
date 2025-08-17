#include "string.h"
#include <stdint.h>
#include <stddef.h>

void    *memcpy(void *to, const void *from, size_t numBytes){
  return memmove(to, from, numBytes);
}

void    *memmove(void *to, const void *from, size_t numBytes){
  uint8_t *d = (uint8_t*)to;
  const uint8_t *s = (const uint8_t*)from;
  size_t n = numBytes;

  if(d == s || n == 0) return to;

  if(d < s){
    // 1) Copy data byte-by-byte until aligned destination
    while((uintptr_t)d%sizeof(uintptr_t) != 0 && n > 0){
      *d++ = *s++;
      n--;
    }

    // 2) Copy data in words
    // Note: use uintptr_t to refer to largest word on system (32 or 64)
    uintptr_t *d_word = (uintptr_t*)d;
    const uintptr_t *s_word = (uintptr_t*)s;
    while(n >= sizeof(uintptr_t)){
      *d_word++ = *s_word++; 
      n-=sizeof(uintptr_t);
    }

    // 3) Copy data in bytes again
    while(n > 0){
      *d++ = *s++;
      n--;
    }
  } else {
    d += n;
    s += n;

    // 1) Copy data byte-by-byte until aligned destination
    while((uintptr_t)d%sizeof(uintptr_t) != 0 && n > 0){
      *--d = *--s;
      n--;
    }

    // 2) Copy data in words
    // Note: use uintptr_t to refer to largest word on system (32 or 64)
    uintptr_t *d_word = (uintptr_t*)d;
    const uintptr_t *s_word = (uintptr_t*)s;
    while(n >= sizeof(uintptr_t)){
      *--d_word = *--s_word; 
      n-=sizeof(uintptr_t);
    }

    // 3) Copy data in bytes again
    while(n > 0){
      *--d = *--s;
      n--;
    }
  }

  return to;
}

int      memcmp(const void *a, const void *b, size_t n){
  if (n == 0) return 0;

  uintptr_t *a_word = (uintptr_t *)a;
  uintptr_t *b_word = (uintptr_t *)b;

  while(n > sizeof(uintptr_t) && *a_word == *b_word){
    a_word++;
    b_word++;
    n-=sizeof(uintptr_t);
  }
  
  uint8_t *a_8 = (uint8_t*)a_word;
  uint8_t *b_8 = (uint8_t*)b_word;
  for(uint8_t i=0; i<sizeof(uintptr_t) && n>0; i++, n--){
    if(a_8[i] != b_8[i]){
      return a_8[i] - b_8[i];
    }
  }

  return 0;
}

void    *memset(void *, int, size_t){  
}
