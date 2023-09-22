#ifndef STDLIB_H
#define STDLIB_H

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

#endif