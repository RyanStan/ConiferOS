#ifndef CONIFER_OS_MEMORY_H
#define CONIFER_OS_MEMORY_H

#include <stddef.h>

/*
 * memset - fill memory with a constant byte
 *
 * fills the first n bytes of the memory area pointed to by s
 * with the constant byte c
 */
void *memset(void *s, int c, size_t n);

/*
 * memcmp - compare memory areas
 *
 * compares the first n bytes (each intepreted as unsigned char) of the memory areas s1 and s2
 * 
 * if return value < 0 then it indicates s1 is less than s2
 * if return value > 0 then it indicates s2 is less than s1
 * if return value == 0 then it indicates s1 is equal to s2
 */
int memcmp(void *s1, void *s2, size_t n);

/* memcpy - copy memory area
 *
 * Copies n bytes from memory area src to memory area dest.
 * The memory areas must not overlap.
 * 
 * Returns a pointer to dest
 */
void *memcpy(void *dest, void *src, size_t n);

#endif /* CONIFER_OS_MEMORY_H */

