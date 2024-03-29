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

/* Exec system call.
 * Constructs a process image from the file and replaces the currently executing process with the new process.
 * This function will return once the process terminates.
 * 
 * argv contains pointers to the command line arguments, and must be null-terminated.
 * 
 * TODO [RyanStan 11-15-23] remove the argc argument
 */
void coniferos_execve(const char *filename, const char *argv[], const int argc);

// exit - cause normal process termination
void coniferos_exit();

#endif