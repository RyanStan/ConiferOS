#ifndef HEAP_H
#define HEAP_H

#include "config.h"
#include <stdint.h>
#include <stddef.h>

/* TODO: create readme that describes the heap algorithm */

#define HEAP_BLOCK_TABLE_ENTRY_FREE 	0x00
#define HEAP_BLOCK_TABLE_ENTRY_TAKEN 	0x01

#define HEAP_BLOCK_IS_FIRST 		0b01000000
#define HEAP_BLOCK_HAS_NEXT 		0b10000000

/* TODO: The goal is to make this heap block table entry type completely opaque,
 * that way it is easy to alter in the future.  Making it opaques requires writing
 * functions for interacting with it.
 * hbte_t = heap block table entry typedef
 */
typedef unsigned char hbte_t;

/* TODO: rename heap entry table to something else.  Entry table is confusing and redundant */
struct heap_entry_table {
	//hbte_t *first_entry;
	hbte_t *entries;
	size_t total_entries;
};

struct heap_desc {
	struct heap_entry_table* table;
	void *start_addr;
};

/*
 * pass in unitialized heap_desc but a valid heap_entry_table.  we will determine if table is valid
 * tbh could make this simpler for caller
 */
int heap_create(struct heap_desc *heap, void *start_addr, void *end_addr, struct heap_entry_table *table);

void* heap_malloc(struct heap_desc *heap, size_t size);

int heap_free(struct heap_desc *heap, void *ptr);

#endif
