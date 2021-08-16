#include "kernel_heap.h"
#include "heap.h"
#include "config.h"
#include "print/print.h"
#include "memory/memory.h"

struct heap_desc kernel_heap;			
struct heap_entry_table kernel_heap_table;

void kernel_heap_init()
{
	/* Initialize kernel heap table 
	 * 
	 * 100 MB kernel heap
	 * 100 MB / 4096 MB = 25600 kernel table entries
	 */
	int rc;
	void *end_addr;

	kernel_heap_table.entries = (hbte_t*)KERNEL_HEAP_TABLE_ADDR;
	kernel_heap_table.total_entries = KERNEL_HEAP_SIZE / HEAP_BLOCK_SIZE;
	
	end_addr = (void*)KERNEL_HEAP_ADDRESS + KERNEL_HEAP_SIZE;
	
	rc = heap_create(&kernel_heap, (void*)KERNEL_HEAP_ADDRESS, end_addr, &kernel_heap_table);
	if (rc < 0) {
		print("Failed to create kernel heap\n");
	}
	
}

void* kmalloc(size_t size)
{
	return heap_malloc(&kernel_heap, size);
}

int kfree(void *ptr)
{
	return heap_free(&kernel_heap, ptr);
}

void* kzalloc(size_t size)
{
	void* ptr = kmalloc(size);
	if (!ptr)
		return 0;

	memset(ptr, 0, size);
	return ptr;	
}
