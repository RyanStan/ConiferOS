#ifndef ISR80H_PROCESS_H
#define ISR80H_PROCESS_H

#include "idt/idt.h"

// Exec system call. Accepts a file name argument and a list of char pointers that represent
// the command-line arguments.
// Loads the file's process image into memory
// and then executes it in user mode.
void *isr80h_command_6_execve(struct interrupt_frame *frame);

// exit - cause normal process termination
void *isr80h_command_7_exit(struct interrupt_frame *frame);

#endif