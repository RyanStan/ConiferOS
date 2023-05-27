#ifndef ISR80H_IO_H
#define ISR80H_IO_H

/* IO Kernel Commands */

struct interrupt_frame;

/* Gets the address of a message from task's stack, and then prints the message */
void *isr80h_command_1_print(struct interrupt_frame *frame);

/* Pop and return character from the current task's process's keyboard buffer.
 * Returns 0x00 if the keyboard buffer is empty.
 */
void *isr80h_command_2_get_key_press(struct interrupt_frame *frame);

#endif