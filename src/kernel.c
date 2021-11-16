#include "kernel.h"
#include "print/print.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kernel_heap.h"
#include "memory/paging/paging.h"
#include "disk/disk.h"
#include "disk/disk_stream.h"
#include "fs/pparser.h"
#include "fs/file.h"
#include "string/string.h"

/* TODO: Create a test file that tests different functionality like paging, the heaps, my file parser, etc... */

void run_smoke_tests()
{
	print("Begin Tests --------------------------\n");
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

	char buf[20];
	strcpy(buf, "hello!");

	/* VFS and FAT16 */
	int fd = fopen("0:/hello.txt", "r");
	if (fd < 0) {
		print("Error opening file\n");	
	} else {
		print("We opened '0:/hello.txt'\n");
		int length = strlen("Hello World");
		char buf[length];
		fseek(fd, 5, SEEK_SET);
		if (fread(buf, 1, length, fd) == 0) {
			print("ERROR: fread\n");
		} else {
			print("Contents of 0:/hello.txt (starting at 5th byte): ");
			print(buf);
			print("\n");
		}
		struct file_stat file_stat;
		fstat(fd, &file_stat);
		if (file_stat.filesize == 12) {
			print("fstat success\n");
		} else {
			print("ERROR: fstat\n");
		}
		fclose(fd);
		print("Closed file descriptor\n");
	}
	print("End Tests --------------------------\n\n");
}

void kernel_main()
{
	terminal_initialize();

	kernel_heap_init();

	fs_init();

	disk_search_and_init();

	idt_init();

	struct paging_desc *paging = init_page_tables(PAGING_READ_WRITE | PAGING_PRESENT | PAGING_USER_SUPERVISOR);
	paging_switch(get_pgd(paging));
	enable_paging();

	enable_interrupts();

	run_smoke_tests();

	print("Welcome to ConiferOS");

	return;
}

