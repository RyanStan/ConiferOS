#include "kernel.h"
#include "print/print.h"

void kernel_main()
{
	/* Since we don't have paging enabled or any sort of virtual memory,
	 * we are literally writing to the address 0xB80000 which is (historically and conventionally) mapped to the video frame buffer by the BIOS
	 * 
	 * side note: I'd be interested in doing more research on this...
	 */

	terminal_initialize();
	print("Welcome to ConiferOS");
	return;
}
