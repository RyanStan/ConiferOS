; Intel syntax
; I know, there are a lot of comments.  I'm a newbie to Intel syntax and x86 assembly 

ORG 0	
BITS 16		; Tells the assembler we are using a 16 bit architecture

_start:		; the following instructions signify the start of the boot record (look at wiki.osdev.org/FAT)
	jmp short start
	nop

times 33 db 0		; creates 33 bytes after previous nop instruction to fill our boot record with arbitrary values

start:
	jmp 0x7c0:step2 	; sets the code segment register to 0x7c0

handle_zero:		; responsible for handling interrupt 0.  We are going to overwrite the 0th entry in the IVT so that this interrupt is ran instead of the divide by 0 code.
	mov ah, 0eh
	mov al, 'A'
	mov bx, 0x00
	int 0x10
	iret

step2:
	cli		; clear interrupts (so that they're disabled while we change segment registers)
	mov ax, 0x7c0
	mov ds, ax	; setup data segment.  note that data segment will start at 0x7c0 * 16 + 0, or 0x7c00, which is the location that BIOS jumps to for execution of bootloader
	mov es, ax	; setup extra segment
	mov ax, 0x00
	mov ss, ax
	mov sp, 0x7c00 ; remember from before: 0x7c00 is the address that RAM will jump to execute bootloader.  ss * 16 + sp = 0x7c00

	sti		; enables interrupts

	mov word[ss:0x00], handle_zero ; if we don't use the stack segment, the data segment will be used.  We want address 0 since that is the start of the 0th entry
				       ; in the interrupt vector table (for interrupt #0).  
				       ; this moves the address of handle_zero into two bytes (a word) starting at physical memory address 0x00 (offset field of the 0th IVT entry).
	mov word[ss:0x02], 0x7c0       ; word[ss:0x02] refers to two bytes at memory address 0x02 which is the segment field of the 0th interrupt vector table entry.

	int 0

	mov si, message ; si stands for source index. 16 bit low end of esi register
	call print
	jmp $		; infinite loop.  $ evaluates to the assembly position at the beginning of this line

print:
	mov bx, 0	; part of interrupt call
.loop:
	lodsb		; loads the character that si register is pointing to into al register.  then increments si register so it points to next char
	cmp al, 0
	je .done
	call print_char
	jmp .loop

.done:			; nasm sub label
	ret

print_char:
	mov ah, 0eh	; function number = 0Eh : Display character
	int 0x10        ; call INT 10h (0x10), BIOS video service 
	ret


message:
	db 'StinkOS is booting...', 0

times 510-($ - $$) db 0 ; pad our code with 0s up to 510 bytes
dw 0xAA55		; Intel machines are little endian so this will be flipped to 0x55AA


