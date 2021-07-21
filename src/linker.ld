ENTRY(kernel_main) 			/* The entry symbol of our output file */
OUTPUT_FORMAT(binary)
SECTIONS
{
	. = 1M; 			       /* We will load our kernel into the byte at 1 MB - sets the location counter to 1MB */
	.text :	ALIGN(4096)		   /* Define the output section .text, which will be at 1 MB in memory */
	{
		*(.text)		       /* all text sections from input object files should be placed into this output section */	
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

	.asm : ALIGN(4096)		/* To not interfere with the text section (compiled c code) alignment, we'll stick our assembly into its own section  */
	{			
		*(.asm)
	}
}