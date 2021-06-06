#ifndef IDT_H
#define IDT_H


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


#endif /* IDT_H */
