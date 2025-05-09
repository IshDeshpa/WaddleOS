#include <stddef.h>
#include <stdint.h>

#define ASSERT(x) do{while(!(x)){}}while(0)

#ifdef __linux__
  #error "ERROR: Must be compiled via cross-compiler"
#elif !defined(__i386)
  #error "ERROR: Must be compiled with an x86-elf compiler"
#endif

// VGA buffer location in x86
// Entries in the VGA buffer take the binary form BBBBFFFFCCCCCCCC, where:
// - B is the background color
// - F is the foreground color
// - C is the ASCII character
volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;

// 80x25
const uint8_t VGA_COLS = 80;
const uint8_t VGA_ROWS = 25;

void vga_write(uint8_t row, uint8_t col, uint8_t bg_4b, uint8_t fg_4b, char c){
  // Bounds check
  if(row > 79 || col > 24) return;

  vga_buffer[(VGA_COLS * row) + col] = ((((uint16_t)(bg_4b & 0xF))<<12) | (((uint16_t)(fg_4b & 0xF))<<8) | (c));
}

void term_clear(){
  for(uint8_t col=0; col<VGA_COLS; col++){
    for(uint8_t row=0; row<VGA_ROWS; row++){
      vga_write(row, col, 0x0, 0xF, ' ');
    }
  }
}

uint8_t term_col = 0;
uint8_t term_row = 0;

void term_putc(char c){
  switch(c){
    case '\n':
      term_col = 0;
      term_row++;
      break;
    default:
      vga_write(term_row, term_col, 0x0, 0xF, c);
      term_col++;
      break;
  }

  if(term_col >= VGA_COLS){
    term_col = 0;
    term_row++;
  }

  if(term_row >= VGA_ROWS){
    term_row = 0;
    term_col = 0;
  }
}

#define MAX_STR_SIZE 500
void term_print(const char* str){
  for(size_t i=0; str[i]!='\0' && i < MAX_STR_SIZE; i++) term_putc(str[i]);
}

void kernel_main(){
  term_clear();

  term_print("Hello, World!\n");
}
