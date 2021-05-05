#ifndef PRINT_H_
#define PRINT_H_

#include <stdint.h>	/* Even in freestanding mode with no std libs, the following libs are available to us, stdint.h and stddef.h */
#include <stddef.h>

uint16_t terminal_make_char(char c, char color);

void terminal_put_char(int row, int col, char c, char color);

void terminal_write_char(char c, char color);

void terminal_initialize();

size_t strlen(const char* str);

void print(const char* str);

#endif /* PRINT_H_ */
