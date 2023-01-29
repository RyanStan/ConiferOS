#ifndef ISR80H_MISC_H
#define ISR80H_MISC_H

/* This file will store various small kernel commands (syscalls) for isr80h. 
 * TODO maybe I should rename this file syscalls, or something similar to make the purpose of this file
 *  more obvious.
 */

struct interrupt_frame;

/* Kernel command for summing two numbers */
void *isr80h_command_0_sum(struct interrupt_frame *frame);

#endif