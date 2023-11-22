/* This linker script will be used to link our object files together */

ENTRY(_start) 				/* The entry symbol of our output file */
OUTPUT_FORMAT(elf32-i386)
SECTIONS
{
	. = 0x400000; 			/* The kernel loads user programs into virtual address 0x400000. All linking should be done with respect to this address. */
	.text :	ALIGN(4096)		/* Define the output section .text, which will be at 1 MB in memory */
	{
		*(.text)			/* all .text input sections from input files should be put into this output section */	
	}

	.asm : ALIGN(4096)		
	{			
		*(.asm)
	}

	.rodata : ALIGN(4096)
	{
		*(.rodata)
	}

	.data : ALIGN(4096)
	{
		*(.data)
	}

	.bss : ALIGN(4096)
	{
		*(COMMON)
		*(.bss)
	}

}
