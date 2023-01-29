#include "idt.h"
#include "memory/memory.h"
#include "config.h"
#include "print/print.h"		// TODO: make some sort of include folder so I don't have to use relative paths in includes
#include "io/io.h"
#include "kernel.h"
#include "task/task.h"
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
extern void int21h_entry();
extern void int_generic_entry();

void int21h_handler()
{
	print("Keyboard pressed\n");
	outb(0x20, 0x20);		// send PIC an acknowledgment
}

void int_generic_handler()
{
	outb(0x20, 0x20);		// send PIC an acknowledgment
}

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

	/* Set interrupt vectors to generic handler */
	for (int i = 0; i < CONIFEROS_TOTAL_INTERRUPTS; i++) {
		idt_set(i, int_generic_entry);
	}
	

	idt_set(0, idt_zero);
	idt_set(0x21, int21h_entry);

	/* Load the interrupt descriptor table */
	idt_load(&idtr);
}

void *isr80h_handle_command(int command, struct interrupt_frame *frame)
{
	return 0;
}

void *isr80h_handler(int command, struct interrupt_frame *frame)
{
	swap_kernel_page_tables(); 								// switch to kernel pages 
	task_current_save_state(frame); 						// save the state of the task that was executing 
	void *res = isr80h_handle_command(command, frame); 		// TODO: implement
	swap_curr_task_page_tables(); 							// switch back to the current_task's pages
	return res;
}