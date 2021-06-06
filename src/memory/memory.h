#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

/*
 * memset - fill memory with a constant byte
 *
 * fills the first n bytes of the memory area pointed to by s
 * with the constant byte c
 */
void *memset(void *s, int c, size_t n);

#endif /* MEMORY_H */

