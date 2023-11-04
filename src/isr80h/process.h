#ifndef ISR80H_PROCESS_H
#define ISR80H_PROCESS_H

#include "idt/idt.h"

// Exec system call. Accepts a file name argument. Loads the file's process image into memory
// and then executes it in user mode.
void *isr80h_command_6_exec(struct interrupt_frame *frame);

#endif