#ifndef HEAP_H
#define HEAP_H

#include "isr80h/io.h"

void *isr80h_command_4_malloc(struct interrupt_frame *frame);

#endif