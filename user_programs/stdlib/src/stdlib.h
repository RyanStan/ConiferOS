#ifndef CONIFEROS_STDLIB_H
#define CONIFEROS_STDLIB_H

#include <stddef.h>

/* See malloc(3)
 * 
 * The malloc() function allocates size bytes and returns a pointer
       to the allocated memory.  The memory is not initialized.  If size
       is 0, then malloc() returns a unique pointer value that can later
       be successfully passed to free().  (See "Nonportable behavior"
       for portability issues.)
 */
void *malloc(size_t size);

/* See free(3)
 * The free() function frees the memory space pointed to by ptr, 
 * which must have been returned by a previous call to malloc(),
 *
 */
void free(void *ptr);

/* Converts a base 10 integer, num, to its (null-terminated) character representation.
 * This function will store the string in buffer, and will return a pointer to it.
 */
char* itoa(int num, char* buffer);

#endif