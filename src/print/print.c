#include "print.h"
#include "string/string.h"

/* The QEMU PC emulator simulates a Cirrus CLGD 5446 PCI VGA card */
#define VGA_WIDTH 				80
#define VGA_HEIGHT 				20
#define BACKSPACE_ASCII_CHAR 	0x08

static uint16_t terminal_row = 0;
static uint16_t terminal_col = 0;

/* The VGA card's frame buffer is memory mapped into the CPU's address space at 0xB8000 
 * (courtesy of the BIOS).
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

// Deletes the last character entered on the display
void terminal_backspace()
{
	if (terminal_row == 0 && terminal_col == 0)
		return;

	if (terminal_col == 0) {
		terminal_row -= 1;
		terminal_col = VGA_WIDTH;
	}

	terminal_col -= 1;
	terminal_write_char(' ', 15);
	terminal_col -= 1;
}

void terminal_write_char(char c, char color)
{

	if (c == '\n') {
		terminal_row++;
		terminal_col = 0;
		return;
	}

	if (c == BACKSPACE_ASCII_CHAR) {
		terminal_backspace();
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

void print(const char* str)
{
	size_t len = strlen(str);
	for (int i = 0; i < len; i++) {
		terminal_write_char(str[i], 15);
	}
}


