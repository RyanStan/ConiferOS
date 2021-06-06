#include "kernel.h"
#include "print/print.h"
#include "idt/idt.h"
 
extern void problem();

void kernel_main()
{
	terminal_initialize();
	idt_init();
	print("Welcome to ConiferOS");
	problem();
	return;
}
