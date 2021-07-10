#ifndef KERNEL_HEAP_H
#define KERNEL_HEAP_H

#include <stddef.h>

void kernel_heap_init();
void* kmalloc(size_t size);
int kfree(void *ptr);

#endif
