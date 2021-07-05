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
	print("Welcome to ConiferOS");

	void *ptr = kmalloc(50);
	void *ptr2 = kmalloc(5000);
	if (ptr || ptr2)

	return;
}
