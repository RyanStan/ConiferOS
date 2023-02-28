#include "kernel.h"
#include "print/print.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kernel_heap.h"
#include "memory/paging/paging.h"
#include "memory/memory.h"
#include "disk/disk.h"
#include "disk/disk_stream.h"
#include "fs/pparser.h"
#include "fs/file.h"
#include "string/string.h"
#include "gdt/gdt.h"
#include "task/tss.h"
#include "task/task.h"
#include "task/process.h"
#include "config.h"
#include "isr80h/isr80h.h"

/* Set up the GDT */
struct tss tss;
struct segment_descriptor_raw gdt_raw[TOTAL_GDT_SEGMENTS];
struct segment_descriptor gdt[TOTAL_GDT_SEGMENTS] = {
	{.base = 0x00, .limit = 0x00, .type = 0x00},					/* NULL segment - used to load other segment registers */
	{.base = 0x00, .limit = 0xffffffff, .type = 0x9A},				/* Kernel code segment. 9A = 1001 1010. P = 0. DPL = 0 S=1 E=1 DC=0 RW=1 */
	{.base = 0x00, .limit = 0xffffffff, .type = 0x92},				/* Kernel data segment. 92 = 1001 0010 DPL=0 RW=1. Offset = Byte 16 (0x10) */
	{.base = 0x00, .limit = 0xffffffff, .type = 0xF8},				/* User code segment. F8 = 1111 1000 DPL=3 RW=0 */
	{.base = 0x00, .limit = 0xffffffff, .type = 0xF2},				/* User data segment. F2 = 1111 0010 DPL=3 RW=1. Offset = Byte 32 (0x20) */
	{.base = (uint32_t)&tss, .limit = sizeof(tss), .type = 0xE9}	/* Task state segment. E9 = 1110 1001 S=0 DPL=3 */
};
#define TSS_GDT_INDEX 5

static struct paging_desc *kernel_pages = 0;

void panic(const char *msg)
{
	print(msg);
	for(;;) {}
}

/* sets ds, es, fs, gs segment registers to the kernel data segment */
void set_seg_regs_to_kernel_data();

void swap_kernel_page_tables()
{
	set_seg_regs_to_kernel_data();
	paging_switch(kernel_pages);
}

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

	memset(gdt_raw, 0x00, sizeof(gdt_raw));
	segment_descriptor_to_raw(gdt_raw, gdt, TOTAL_GDT_SEGMENTS);
	gdt_load(gdt_raw, sizeof(gdt_raw));	

	kernel_heap_init();

	fs_init();

	disk_search_and_init();

	idt_init();

	/* Load the task register with segment selector for tss */
	memset(&tss, 0, sizeof(tss));
	tss.esp0 = KERNEL_STACK_ADDR;
	tss.ss0 = KERNEL_DATA_SELECTOR;
	tss_load(TSS_GDT_INDEX * sizeof(struct segment_descriptor_raw));

	kernel_pages = init_page_tables(PAGING_READ_WRITE | PAGING_PRESENT | PAGING_USER_SUPERVISOR);
	paging_switch(kernel_pages);
	enable_paging();

	isr80h_register_commands();

	run_smoke_tests();

	print("Welcome to ConiferOS\n");

	struct process *process = 0;
	int rc = process_load("0:/print.bin", &process);
	if (rc < 0)
		panic("Failed to load 0:/print.bin\n");

	// enable_interrupts(); TODO: task run code expects interrupts to be disabled...

	task_exec(get_task_list_head());

	return;
}

