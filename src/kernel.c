#include "kernel.h"
#include "print/print.h"
#include "idt/idt.h"
#include "io/io.h"
 

void kernel_main()
{
	terminal_initialize();
	idt_init();
	print("Welcome to ConiferOS");

	return;
}
