[BITS 32]		; all code below here is seen as 32-bit code.  The brackets indicate a primitive directive
global _start
extern kernel_main

; should be in text section - must be very first code in our kernelfull object file

CODE_SEG equ 0x08	; these are the offsets into the GDT for the respective segment descriptors
DATA_SEG equ 0x10

_start:
	mov ax, DATA_SEG	
	mov ds, ax 	; Set the rest of our segment register to point to the 3rd entry in our gdt (gdt_data).
	mov es, ax	; We use the same segment descriptor for all these segments since we'll be using paging instead of segmentation
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov ebp, 0x00200000	; arbitrarily set the stack and base pointer further in memory (2 MB) since now in protected mode we can access more memory
	mov esp, ebp

	; We are now running in protected mode!
	
	; Enable the A20 line
	in al, 0x92		; for description of in and out instructions, see the chapter on input/output in the IA-32 Software Developer's Manual
	or al, 2		; in reads from a port and out writes to a port (actually writes to IO address space I believe - specific port is a feature of the chipset)
	out 0x92, al
	
	call kernel_main
	jmp $

; since this assembly file will be in the first section of our final linked executable, we need it to be properly aligned so that the 
; following compiled c code works and is also properly aligned
times 512-($ - $$) db 0
