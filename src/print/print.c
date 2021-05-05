#include "print.h"

/* The QEMU PC emulator simulates a Cirrus CLGD 5446 PCI VGA card */
#define VGA_WIDTH 80
#define VGA_HEIGHT 20

/* TODO: add function comments and make things more clear (i.e. the names of functions) */

/* Since we don't have paging enabled or any sort of virtual memory,
 * we are literally writing to the address 0xB80000 which is (historically and conventionally) mapped to the video frame buffer by the BIOS
 * 
 */
static uint16_t* video_mem = (uint16_t*)(0xB8000);

uint16_t terminal_make_char(char c, char color)
{
	/* Since CPU is little endian, the value we're writing 
	 * to the frame buffer should have the color as the msb 
	 * and the character as the lsb
	 */
	return (color << 8) | c;
}


void terminal_put_char(int row, int col, char c, char color)
{
	video_mem[row * VGA_WIDTH + col] = terminal_make_char(c, color);
}

void terminal_write_char(char c, char color)
{
	static uint16_t terminal_row = 0;
	static uint16_t terminal_col = 0;

	if (c == '\n') {
		terminal_row++;
		terminal_col = 0;
		return;
	}

	terminal_put_char(terminal_row, terminal_col, c, color);

	terminal_col++;
	if (terminal_col >= VGA_WIDTH) {
		terminal_row++;
		terminal_col = 0;
	}
}

void terminal_initialize() 
{
	for (int row = 0; row < VGA_HEIGHT; row++) {
		for (int col = 0; col < VGA_WIDTH; col++) {
			terminal_put_char(row, col, ' ', 0);
		}
	}
}


size_t strlen(const char* str)
{
	size_t len = 0;

	while (str[len] != 0) {
		len++;
	}

	return len;
}

void print(const char* str)
{
	size_t len = strlen(str);
	for (int i = 0; i < len; i++) {
		terminal_write_char(str[i], 15);
	}
}


