; Intel syntax
; I know, there are a lot of comments.  I'm a newbie to Intel syntax and x86 assembly 

ORG 0x7c00				; This is where the program expects to be loaded into memory
BITS 16					; Tells the assembler we are using a 16 bit architecture

CODE_SEG equ gdt_code - gdt_start	; EQU is a NASM psuedo instruction that gives a symbol (CODE_SEG) a corresponding value (gdt_code - gdt_start)
DATA_SEG equ gdt_data - gdt_start	; These symbols will be used to give us our offsets into the GDT for the respective segment descriptors

_start:					; the following instructions signify the start of the boot record (look at wiki.osdev.org/FAT)
	jmp short start
	nop

times 33 db 0				; creates 33 bytes after previous nop instruction to fill our boot record with arbitrary values

start:
	jmp 0x0:step2			; sets the code segment register to 0x0.  This will actually be address 0x7c00 since we set ORG 0x7c00

step2:
	cli				; clear interrupts (so that they're disabled while we change segment registers)
	mov ax, 0x0
	mov ds, ax			; setup data segment.  note that data segment will start at 0x7c0 * 16 + 0, or 0x7c00, which is the location that BIOS jumps to for execution of bootloader
	mov es, ax			; setup extra segment
	mov ss, ax
	mov sp, 0x7c00 			; remember from before: 0x7c00 is the address that RAM will jump to execute bootloader.  ss * 16 + sp = 0x7c00
	sti				; enables interrupts



.load_protected:			; load the processor into protected mode
	cli
	lgdt[gdt_descriptor]
	mov eax, cr0
	or al, 0x1			; set PE (Protection Enable) bit in CR0 (control register 0)
	mov cr0, eax
	jmp CODE_SEG:load32 		; sets the code segment selector to CODE_SEG and jumps to load32

; GDT
gdt_start:
gdt_null:
	dd 0x0 				; 4 bytes of 0s
	dd 0x0 				; This will represent a null entry (null segment descriptor) in our gdt

; offset 0x8 - this is the offset in our GDT where we will make the code segment descriptor entry (will be the 2nd entry in our gdt since each entry is 8 bytes)
gdt_code:				; CS SHOULD POINT TO THIS
	dw 0xffff 			; Segment Limit 0:15 bits.  This is the maximum addressable unit for this segment (page granularity, 4 KiB in this case)
	dw 0				; Base 0:15.  The base describes the linear address where the segment begins
	db 0				; Base 16:23 bits
	db 0x9a				; Access byte
	db 11001111b			; High 4 bit flags and low 4 bit flags
	db 0				; Base 24-31 bits

; offset 0x10				; (3rd entry in our gdt - we're actually using this for DS, SS, ES, FS, and GS since we're not really using segmentation anyways)
gdt_data:				; DS, SS, ES, FS, GS
	dw 0xffff 			; Segment Limit 0:15 bits.  This is the maximum addressable unit for this segment (page granularity, 4 KiB in this case)
	dw 0				; Base 0:15.  The base describes the linear address where the segment begins
	db 0				; Base 16:23 bits
	db 0x92				; Access byte
	db 11001111b			; High 4 bit flags and low 4 bit flags
	db 0				; Base 24-31 bits

gdt_end:

gdt_descriptor:				; This is the GDT description structure that will be used by the lgdt instruction
	dw gdt_end - gdt_start - 1	; Size
	dd gdt_start			; Offset - linear address of the GDT itself

[BITS 32]				; all code below here is seen as 32-bit code.  The brackets indicate a primitive directive

load32:
	mov ax, DATA_SEG	
	mov ds, ax 			; Set the rest of our segment register to point to the 3rd entry in our gdt (gdt_data).
	mov es, ax			; We use the same segment descriptor for all these segments since we'll be using paging instead of segmentation
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov ebp, 0x00200000		; arbitrarily set the stack and base pointer further in memory (2 MB) since now in protected mode we can access more memory
	mov esp, ebp

	; We are now running in protected mode!
	
	; Enable the A20 line
	in al, 0x92			; for description of in and out instructions, see the chapter on input/output in the IA-32 Software Developer's Manual
	or al, 2			; in reads from a port and out writes to a port (actually writes to IO address space I believe - specific port is a feature of the chipset)
	out 0x92, al
	
	jmp $


times 510-($ - $$) db 0 		; pad our code with 0s up to 510 bytes
dw 0xAA55				; Intel machines are little endian so this will be flipped to 0x55AA

