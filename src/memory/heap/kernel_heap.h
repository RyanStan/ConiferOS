/*
 * kernel_heap
 * implementation of heap interface
 */

#ifndef KERNEL_HEAP_H
#define KERNEL_HEAP_H

#include <stddef.h>

/* Initialize the kernel heap */
void kernel_heap_init();

/* Allocate size bytes from the heap and return a pointer to first allocated block */
void* kmalloc(size_t size);

/* Free allocated memory from the heap at ptr */
int kfree(void *ptr);

/* Allocate size bytes from the heap and zero them */
void* kzalloc(size_t size);

#endif
