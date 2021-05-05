#include "kernel.h"
#include "print/print.h"

void kernel_main()
{
	terminal_initialize();
	print("Welcome to ConiferOS");
	return;
}
