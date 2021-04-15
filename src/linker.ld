/* This linker script will be used to link our object files together */

ENTRY(_start) 			/* The entry symbol of our output file */
OUTPUT_FORMAT(binary)
SECTIONS
{
	. = 1M; 		/* We will load our kernel into the byte at 1 MB - sets the location counter to 1MB */
	.text :			/* Define the output section .text, which will be at 1 MB in memory */
	{
		*(.text)	/* all .text input sections from input files should be put into this output section */	
	}

	.rodata :
	{
		*(.rodata)
	}

	.data :
	{
		*(.data)
	}

	.bss :
	{
		*(COMMON)
		*(.bss)
	}
}
