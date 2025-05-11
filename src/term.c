#include <stdint.h>
#include <stddef.h>
#include "term.h"

// VGA buffer location in x86
// Entries in the VGA buffer take the binary form BBBBFFFFCCCCCCCC, where:
// - B is the background color
// - F is the foreground color
// - C is the ASCII character
volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;

// 80x25
const uint8_t VGA_COLS = 80;
const uint8_t VGA_ROWS = 25;

static inline void _vga_write(uint8_t row, uint8_t col, uint16_t raw){
  // Bounds check
  if(row > 79 || col > 24) return;

  vga_buffer[(VGA_COLS * row) + col] = raw; 
}

static inline void vga_write(uint8_t row, uint8_t col, uint8_t bg_4b, uint8_t fg_4b, char c){
  _vga_write(row, col, (((uint16_t)(bg_4b & 0xF))<<12) | (((uint16_t)(fg_4b & 0xF))<<8) | (c));
}

static inline uint16_t _vga_read(uint8_t row, uint8_t col){
  // Bounds check
  if(row > 79 || col > 24) return 0;

  return vga_buffer[(VGA_COLS * row) + col];
}

static inline char vga_readc(uint8_t row, uint8_t col){
  return _vga_read(row, col) & 0xFF;
}

static inline uint8_t vga_readbg(uint8_t row, uint8_t col){
  return (_vga_read(row, col) >> 8) & 0xF;
}

static inline uint8_t vga_readfg(uint8_t row, uint8_t col){
  return (_vga_read(row, col) >> 12) & 0xF;
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
    // Shift all rows up one
    for(uint8_t col=0; col<VGA_COLS; col++){
      for(uint8_t row=1; row<VGA_ROWS; row++){
	_vga_write(row-1, col, _vga_read(row, col));
      }
    }
    term_col = 0;
    term_row = VGA_ROWS-1;
  }
}

#define MAX_STR_SIZE 500
void term_print(const char* str){
  for(size_t i=0; str[i]!='\0' && i < MAX_STR_SIZE; i++) term_putc(str[i]);
}
