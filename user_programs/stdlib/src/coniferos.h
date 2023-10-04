#ifndef CONIFER_OS_H
#define CONIFER_OS_H

#include <stddef.h>

// C interface to ConiferOS system calls

void print(const char *filename);

int get_key();

void *coniferos_malloc(size_t size);

void coniferos_free(void *ptr);

void coniferos_putchar(char c);

#endif