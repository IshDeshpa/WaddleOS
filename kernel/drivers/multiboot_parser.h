#ifndef MULTIBOOT_PARSER_H
#define MULTIBOOT_PARSER_H

#include "multiboot2.h"
#include <stdint.h>

struct multiboot_tag *multiboot_get_tag(uint32_t multiboot_tag_type);

#endif
