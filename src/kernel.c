#include "kernel.h"
#include "print/print.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kernel_heap.h"
 

void kernel_main()
{
	terminal_initialize();
	kernel_heap_init();
	idt_init();
	enable_interrupts();
	print("Welcome to ConiferOS");

	return;
}
