#include "kernel.h"
#include "print/print.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kernel_heap.h"
#include "memory/paging/paging.h"
#include "disk/disk.h"
 
static struct paging_desc* paging;

void kernel_main()
{
	terminal_initialize();

	kernel_heap_init();

	idt_init();

	paging = init_page_tables(PAGING_READ_WRITE | PAGING_PRESENT | PAGING_USER_SUPERVISOR);
	paging_switch(get_pgd(paging));

	/* Example to make sure paging works 
	 * vaddr 0x1000 will point to the allocated memory at ptr
	 */
	char *ptr = kzalloc(PAGING_PAGE_SIZE);
	paging_set(get_pgd(paging), (void*)0x1000, (uint32_t)ptr | PAGING_USER_SUPERVISOR | PAGING_PRESENT | PAGING_READ_WRITE);

	enable_paging();

	/* Test disk read */
	char buf[DISK_SECTOR_SIZE];
	disk_read_sector(0, 1, buf);

	enable_interrupts();

	print("Welcome to ConiferOS");

	return;
}
