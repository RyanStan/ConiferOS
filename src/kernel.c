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
	void *ptr3 = kmalloc(5600);
	kfree(ptr);
	void *ptr4 = kmalloc(50);

	if (ptr || ptr2 || ptr3 || ptr4)
	/* Since we freed ptr, we should see that ptr4 points to the same address that 
         * ptr initially pointed to.  This is just a way to test kmalloc and kfree
	 */

	return;
}
