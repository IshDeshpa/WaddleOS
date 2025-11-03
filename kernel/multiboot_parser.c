#include "multiboot2.h"
#include <stdbool.h>
#include <stdint.h>

extern uint32_t mboot_tags;
struct multiboot_tag *multiboot_get_tag(uint32_t multiboot_tag_type){
  // Start at mboot_tags + 8
  struct multiboot_tag *tag = (struct multiboot_tag *)((uintptr_t)mboot_tags + 8);
  while (tag->type != 0) { // 0 = end tag
    if (tag->type == multiboot_tag_type) {
      return tag; // found
    }

    // Move to next tag (8-byte aligned)
    tag = (struct multiboot_tag *)(((uintptr_t)tag + tag->size + 7) & ~7);
  }

  return 0; // not found
}
