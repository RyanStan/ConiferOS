#ifndef ISR80H_IO_H
#define ISR80H_IO_H

/* IO Kernel Commands */

struct interrupt_frame;

/* Gets the address of a message from task's stack, and then prints the message */
void *isr80h_command_1_print(struct interrupt_frame *frame);

#endif