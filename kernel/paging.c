#include "paging.h"
#include "term.h"
#include <stddef.h>

void *paging_palloc(){
   
  return NULL;
}

void paging_init(){
  term_printf("Page entry size: %d\n\r", sizeof(pml4e_t));
}
