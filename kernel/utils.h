#ifndef UTILS_H
#define UTILS_H

#define ASSERT(x) do{while(!(x)){  __asm__ volatile("hlt"); }}while(0)

#endif
