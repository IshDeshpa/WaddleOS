#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "waddletest.h"
#include "test_defs.h"

#define X(testname) bool testname();
TEST_LIST
#undef X

#define MAX_INDENTS 5 
char indents[MAX_INDENTS]="\0";
uint8_t indent_level=0;

void inc_indent_level(){
  if(indent_level < MAX_INDENTS){
    indents[indent_level] = '\t';
    indent_level++;
  }
}

void dec_indent_level(){
  if(indent_level > 0){
    indent_level--;
    indents[indent_level] = '\0';
  }
}

int main(){
  bool passing = true;

  printf("%sRunning %s ...\n\r", indents, __FILE_NAME__);

  #define X(testname) \
  printf("%sRunning %s ...\n\r", indents, #testname); \
  passing = passing && testname(); \
  if (!passing) return 0;

  TEST_LIST

  #undef X
}
