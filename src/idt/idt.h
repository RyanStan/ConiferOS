#ifndef IDT_H
#define IDT_H

#include <stdint.h>

struct interrupt_frame
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t unused_esp;    // This is the value of the esp before the pushad operation appended everything to the stack
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t ip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;           // This is the value of the esp after the pushad operation appended everything to the stack
    uint32_t ss;
} __attribute__((packed));

/* ISR80H_COMMAND is a pointer to a function that accepts an interrupt frame and returns a void pointer.
 * This type will represent the various kernel service routines that user programs can execute by calls to interrupt 0x80.
 */
typedef void* (*ISR80H_COMMAND)(struct interrupt_frame *frame);

// Used to define interrupt handlers that can be registered with idt_register_interrupt_handler.
typedef void (*INTERRUPT_HANDLER)();

/* 
 * idt_zero - handler for interrupt 0
 */
void idt_zero();

/*
 * idt_set - set the ith entry in the idt with the given handler function
 *
 * i - the interrupt number
 * handler - address of the interrupt/exception handler 
 */ 
void idt_set(int i, void *handler);

void idt_init();

void enable_interrupts();

void disable_interrupts();

/* Interrupt handler for int 0x80.  The kernel calls `int 0x80` when it wants to invoke
 * a kernel command.  The specific command from userland is passed via eax and is represented by the command parameter here.
 * The interrupt_frame stores the registers that were passed to the kernel on the stack when
 * the user program issued the `int` instruction (that instruction puts them on the stack for us).
 */
void *isr80h_handler(int command, struct interrupt_frame *frame);

/* Called from kernel land.  Registers the command
 * so that it can be called from userland via int 0x80.
 * TODO: move this to isr80h.h.  
 */
void isr80h_register_command(int command_id, ISR80H_COMMAND command);

/* Calls the appropriate interrupt handler for the interrupt # sent to us from the PIC.
 * To make an interrupt handler visible to this routine, you must register it via 
 * idt_register_interrupt_handler.
 */
void interrupt_handler(int interrupt, struct interrupt_frame *frame);

// Register a C routine, interrupt_handler, to handle the given interrupt number.
int idt_register_interrupt_handler(int interrupt, INTERRUPT_HANDLER interrupt_handler);

#endif /* IDT_H */
