#include "kernel.h"
#include "print/print.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kernel_heap.h"
#include "memory/paging/paging.h"
 
static struct paging_desc* paging;

void kernel_main()
{
	terminal_initialize();

	kernel_heap_init();

	idt_init();

	paging = init_page_tables(PAGING_READ_WRITE | PAGING_PRESENT | PAGING_USER_SUPERVISOR);
	paging_switch(get_pgd(paging));
	enable_paging();

	enable_interrupts();

	print("Welcome to ConiferOS");

	return;
}
