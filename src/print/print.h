#ifndef PRINT_H_
#define PRINT_H_

#include <stdint.h>	/* Even in freestanding mode with no std libs, the following libs are available to us, stdint.h and stddef.h */
#include <stddef.h>

uint16_t terminal_make_char(char c, char color);

void terminal_put_char(int row, int col, char c, char color);

// Write a char to the next available row,col of the VGA framebuffer
void terminal_write_char(char c, char color);

void terminal_initialize();

void print(const char* str);

#endif /* PRINT_H_ */
