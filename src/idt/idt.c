#include "idt.h"
#include "memory/memory.h"
#include "config.h"
#include "../print/print.h"		// TODO: make some sort of include folder so I don't have to use relative paths in includes
#include <stdint.h>

#define CONIFEROS_TOTAL_INTERRUPTS 256	


struct idt_entry {			// idt entry for interreupt gate descriptor
	uint16_t offset_1; 
	uint16_t selector;
	uint8_t zero;
	uint8_t type_attr;
	uint16_t offset_2;
} __attribute__((packed));

struct idtr_desc {			// format of idtr register
	uint16_t limit;			// size of the idt - 1 (in bytes)
	uint32_t base;			// Base address of the idt
} __attribute__((packed));

static struct idt_entry idt[CONIFEROS_TOTAL_INTERRUPTS];
static struct idtr_desc idtr;

extern void idt_load(struct idtr_desc  *val);

/* 
 * idt_zero - handler for interrupt 0
 */
void idt_zero()
{
	print("Divide by zero error\n");
}


/*
 * idt_set - set the ith entry in the idt with the given handler function
 *
 * i - the interrupt number
 * handler - address of the interrupt/exception handler 
 */ 
void idt_set(int i, void *handler)
{
	struct idt_entry *entry = &idt[i];
	entry->offset_1 = (uint32_t)handler & 0x0000ffff;
	entry->selector = KERNEL_CODE_SELECTOR;
	entry->zero = 0x00;
	entry->type_attr = 0xEE;
	entry->offset_2 = (uint32_t)handler >> 16;
}

void idt_init()
{
	memset(idt, 0, sizeof(idt));	
	idtr.limit = sizeof(idt) - 1;
	idtr.base = (uint32_t)idt;

	idt_set(0, idt_zero);

	/* Load the interrupt descriptor table */
	idt_load(&idtr);
}
