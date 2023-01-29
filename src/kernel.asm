[BITS 32]		; all code below here is seen as 32-bit code.  The brackets indicate a primitive directive
global _start
global  problem
global set_seg_regs_to_kernel_data
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

	; Remap the master PIC (Assuming that QEMU is emulating a 8259 PIC for us as opposed to a more modern I/O and local APICs)
	mov al, 00010001b	
	out 0x20, al		; send 10001b (initialization mode command) to I/O port 0x20 which is Master PIC - Command

	mov al, 0x20 		; Interrupt 0x20 is where master ISR should start
	out 0x21, al

	mov al, 00000001b	
	out 0x21, al		; End initialization mode
	; End remap of the master PIC
	
	call kernel_main
	jmp $


set_seg_regs_to_kernel_data:
    mov ax, 0x10                ; 0x10 is the offset into our gdt for the kernel data segment (see also KERNEL_DATA_SEGMENT). 
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret


; since this assembly file will be in the first section of our final linked executable, we need it to be properly aligned so that the 
; following compiled c code works and is also properly aligned
times 512-($ - $$) db 0
