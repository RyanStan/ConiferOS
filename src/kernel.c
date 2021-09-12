#include "kernel.h"
#include "print/print.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kernel_heap.h"
#include "memory/paging/paging.h"
#include "disk/disk.h"
#include "disk/disk_stream.h"
#include "fs/pparser.h"

/* TODO: Create a test file that tests different functionality like paging, the heaps, my file parser, etc... */

void run_tests()
{
	/* Test path parsing */
	struct path_root *root_path = pparser_parse("0:/bin/shell.bin", NULL);
	if (root_path)
		print("Parse path worked\n");

	/* Test disk streamer 
	 * View disk image binary and confirm that values read into c are correct
	 */
	struct disk_stream *disk_stream = get_disk_stream(0);			
	int rc = disk_stream_seek(disk_stream, 0x201);
	if (rc < 0)
		print("Error on disk_stream_seek\n");
	unsigned char c = 0;
	rc = disk_stream_read(disk_stream, &c, 1); 		

	unsigned char c_buf[4];
	rc = disk_stream_seek(disk_stream, 511);
	disk_stream_read(disk_stream, c_buf, 4);
}

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

	run_tests();

	print("Welcome to ConiferOS");

	return;
}

