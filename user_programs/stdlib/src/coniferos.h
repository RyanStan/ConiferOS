#ifndef CONIFER_OS_H
#define CONIFER_OS_H

#include <stddef.h>

// C interface to ConiferOS system calls

void print(const char *filename);

int coniferos_get_key();

// Blocks until a key is pressed. Then returns the value of the key.
int coniferos_get_key_blocking();

void *coniferos_malloc(size_t size);

void coniferos_free(void *ptr);

void coniferos_putchar(char c);

/* Reads user input from the keyboard until carriage returns,
 * and then stores input in the out buffer. 
 *
 * This will print the characters to
 * the screen as they're typed.
 * 
 * max is the max length of the out buffer.
 */
int coniferos_terminal_readline(char *out, int max);

#endif