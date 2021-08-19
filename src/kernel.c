#include "kernel.h"
#include "print/print.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kernel_heap.h"
#include "memory/paging/paging.h"
#include "disk/disk.h"

void kernel_main()
{
	terminal_initialize();

	kernel_heap_init();

	disk_search_and_init();

	idt_init();

	struct paging_desc *paging = init_page_tables(PAGING_READ_WRITE | PAGING_PRESENT | PAGING_USER_SUPERVISOR);
	paging_switch(get_pgd(paging));
	enable_paging();

	enable_interrupts();

	print("Welcome to ConiferOS");

	return;
}
