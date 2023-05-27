#include "idt.h"
#include "memory/memory.h"
#include "config.h"
#include "print/print.h"		// TODO: make some sort of include folder so I don't have to use relative paths in includes
#include "io/io.h"
#include "kernel.h"
#include "task/task.h"
#include <stdint.h>
#include "status.h"

#define CONIFEROS_TOTAL_INTERRUPTS 512	

/* Each kernel routine that a user program can invoke via interrupt 0x80 will be stored here */
static ISR80H_COMMAND isr80h_commands[MAX_ISR80H_COMMANDS];

// Registered interrupt handlers.
static INTERRUPT_HANDLER interrupt_handlers[CONIFEROS_TOTAL_INTERRUPTS];

// An array of pointers is defined in idt.asm. The array will contain the addresses
// of the assembly functions which handle the corresponding interrupt number.
// Example: index 5 of this array will contain the interrupt handler for interrupt #5.
extern void *interrupt_pointer_table[CONIFEROS_TOTAL_INTERRUPTS];

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
extern void isr80h_wrapper();

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
		idt_set(i, interrupt_pointer_table[i]);
	}
	
	// Overwrite handler for the interrupts which require special handling
	idt_set(0, idt_zero);
	idt_set(0x80, isr80h_wrapper);

	/* Load the interrupt descriptor table */
	idt_load(&idtr);
}

void isr80h_register_command(int command_id, ISR80H_COMMAND command)
{
	if (command_id < 0 || command_id >= MAX_ISR80H_COMMANDS)
		panic("The command_id is out of bounds\n");

	if (isr80h_commands[command_id])
		panic("You're attempting to overwrite an existing command\n");

	isr80h_commands[command_id] = command;
}

/* Invokes the system call (ISR80H_COMMAND) that is associated with command and passes it frame */
void *isr80h_handle_command(int command_id, struct interrupt_frame *frame)
{
	if (command_id < 0 || command_id >= MAX_ISR80H_COMMANDS)
		return 0;

	ISR80H_COMMAND command_func = isr80h_commands[command_id];
	if (!command_func) 
		return 0;

	void *result = command_func(frame);
	return result;
}

void *isr80h_handler(int command_id, struct interrupt_frame *frame)
{
	swap_kernel_page_tables(); 								// switch to kernel pages 
	task_current_save_state(frame); 						// save the state of the task that was executing 
	void *res = isr80h_handle_command(command_id, frame);
	swap_curr_task_page_tables(); 							// switch back to the current_task's pages
	return res;
}

// Generic interrupt handler. Calls the appropriate interrupt handler for the interrupt # sent to us from the PIC.
void interrupt_handler(int interrupt, struct interrupt_frame *frame)
{
	swap_kernel_page_tables(); 								// switch to kernel pages
	if (interrupt_handlers[interrupt] != 0) {
		task_current_save_state(frame); 					// save the state of the task that was executing
		interrupt_handlers[interrupt]();
	}
	swap_curr_task_page_tables(); 							// switch back to the current_task's pages
	outb(0x20, 0x20);										// send PIC an acknowledgment
}

int idt_register_interrupt_handler(int interrupt, INTERRUPT_HANDLER interrupt_handler)
{
	if (interrupt < 0 || interrupt >= CONIFEROS_TOTAL_INTERRUPTS)
		return -EINVARG;

	interrupt_handlers[interrupt] = interrupt_handler;
	return 0;
}