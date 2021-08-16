#include "heap.h"
#include "status.h"
#include "memory/memory.h"

/* Ensures that pointer is aligned on boundary divisible by the size of a block in our heap
 * Returns 1 (true) when ptr is aligned correctly and 0 (false) when it is not
 */
static int heap_valid_alignment(void *ptr)
{
	return (uintptr_t)ptr % HEAP_BLOCK_SIZE == 0;
}

/* Takes in pointer to a heap, end address of the heap,and a pointer to the corresponding heap entry table.
 * Returns 1 (true) if the table has the correct amount of entries or 0 (false)  otherwise
 */
static int heap_valid_table(void *heap_start_addr, void *end_addr, struct heap_entry_table *table)
{
	/* TODO: it would be much better if we just created the table ourselves instead 
	 * of forcing the user to do it
	 */
	size_t heap_size;
	size_t heap_total_blocks;

	heap_size = (size_t)(end_addr - heap_start_addr);	
	heap_total_blocks = heap_size / HEAP_BLOCK_SIZE;
	if (heap_total_blocks != table->total_entries) {
		return FALSE;
	}

	return TRUE;
}

int heap_create(struct heap_desc *heap, void *start_addr, void *end_addr, struct heap_entry_table *table)
{
	size_t table_size;

	if (!(heap_valid_alignment(start_addr) && heap_valid_alignment(end_addr))) {
		return -EINVARG;
	}

	memset(heap, 0, sizeof(struct heap_desc));
	heap->start_addr = start_addr;
	heap->table = table;

	if (!heap_valid_table(start_addr, end_addr, table)) {
		return -EINVARG;
	}
	
	/* Initialize all blocks in the heap entry table to 0 to indicate each block in the heap is free */
	table_size = sizeof(hbte_t) * table->total_entries;
	memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);
	return 0; // 0 = success, < 0 = failure error code
}

/* Takes in an size and aligns it to the upper heap block size boundary.
 * E.g. if heap block size is 4096 bytes and 5000 is passed in, then the value returned is 
 * 8192
 */
static uint32_t align_upper_block_boundary(uint32_t val) 
{
	if (val % HEAP_BLOCK_SIZE == 0) {
		return val;
	}

	val = val - (val % HEAP_BLOCK_SIZE) + HEAP_BLOCK_SIZE;
	return val;
}

/* See the heap readme to better understand what this function is doing */
static int heap_get_entry_type(hbte_t entry)
{
	return entry & 0x0f;
}

/* 
 * Look through heap entry table and see it can find enough room for total_blocks blocks
 * Returns index of the start block on success, or < 0 on failure
 */
int heap_get_start_block_index(struct heap_desc *heap, size_t total_blocks)
{
	struct heap_entry_table *table;
	int bs;					/* stores index of the first free suitable array of blocks */
	int bc;					/* stores the current block that the algorithm is on */

	table = heap->table;
	bs = -1;
	bc = 0;

	for (size_t i = 0; i < heap->table->total_entries; i++) {

		if (heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_TABLE_ENTRY_FREE) {
			bc = 0;
			bs = -1;
			continue;
		}

		/* If this is the first block */
		if (bs == -1) {
			bs = i;
		}

		bc++;
		if (bc == total_blocks) {
			break;
		}
	}

	if (bs == -1)
		return -ENOMEM;

	return bs;
}

/*
 * heap_block_to_address 
 * calculates the start address for block at block_index in heap
 */
void* heap_block_to_address(struct heap_desc *heap, uint32_t block_index)
{
	return heap->start_addr + (block_index * HEAP_BLOCK_SIZE);
}

/*
 * heap_address_to_block
 * calculates the block index of the block in heap at addr
 */
int heap_address_to_block(struct heap_desc *heap, void *addr)
{
	return (int)(addr - heap->start_addr) / HEAP_BLOCK_SIZE;
}

/*
 * heap_mark_blocks_taken
 * Update the heap entry table corresponding with heap so that
 * total_blocks starting at block_index are marked as taken with the correct flags as described
 * in the heap readme
 */
int heap_mark_blocks_taken(struct heap_desc *heap, int start_block_index, size_t total_blocks)
{
	int end_block_index;
	hbte_t entry;

	end_block_index = start_block_index + total_blocks - 1;
	entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;
	if (total_blocks > 1) {
		entry |= HEAP_BLOCK_HAS_NEXT;
	}

	for (int i = start_block_index; i <= end_block_index; i++) {
		heap->table->entries[i] = entry;
		entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;
		if (i != end_block_index - 1) {
			entry |= HEAP_BLOCK_HAS_NEXT;
		}
	}

	return 0;
}

int heap_mark_blocks_free(struct heap_desc *heap, int start_block_index)
{
	hbte_t entry;
	int i;
	
	for (i = start_block_index; i < (int)heap->table->total_entries; i++) {
		entry = heap->table->entries[i];
		heap->table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE;

		if (!(entry & HEAP_BLOCK_HAS_NEXT)) 
			break;
	}
	return 0;
}

void* heap_malloc_blocks(struct heap_desc *heap, size_t total_blocks)
{
	void *addr;
	int start_block;

	start_block = heap_get_start_block_index(heap, total_blocks); 
	if (start_block < 0) {
		return NULL;
	}

	
	addr = heap_block_to_address(heap, start_block);			

	heap_mark_blocks_taken(heap, start_block, total_blocks);

	return addr;
}

void* heap_malloc(struct heap_desc *heap, size_t size)
{
	size_t aligned_size = align_upper_block_boundary(size);
	size_t total_blocks = aligned_size / HEAP_BLOCK_SIZE;
	return heap_malloc_blocks(heap, total_blocks);
}

int heap_free(struct heap_desc *heap, void *ptr)
{
	heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));
	return 0;
}