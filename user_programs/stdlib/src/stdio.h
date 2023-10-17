#ifndef CONIFEROS_STDIO
#define CONIFEROS_STDIO

// Library to perform input and output operations

/* Write a character to the display
 * TODO [ryanstan 10/3/23] Implement the concept of stdout, and modify this
 * function to write to stdout instead of directly to the terminal hardware device. 
 */
int putchar (int character);

/* Writes the C string pointed by format to the terminal.
 * If format includes format specifiers (subsequences beginning with %),
 * the additional arguments following format are formatted and inserted in the resulting string
 * replacing their respective specifiers
 * %s = string
 * %i = integer (can be max 24 digits)
 */
int printf(const char *fmt, ...);

#endif