; Intel syntax
; I know, there are a lot of comments.  I'm a newbie to Intel syntax and x86 assembly 

ORG 0x7c00			; This is where the program expects to be loaded into memory
BITS 16				; Tells the assembler we are using a 16 bit architecture

CODE_SEG equ gdt_code - gdt_start	; EQU is a NASM psuedo instruction that gives a symbol (CODE_SEG) a corresponding value (gdt_code - gdt_start)
DATA_SEG equ gdt_data - gdt_start	; These symbols will be used to give us our offsets into the GDT for the respective segment descriptors

_start:				; the following instructions signify the start of the boot record (look at wiki.osdev.org/FAT)
	jmp short start
	nop

times 33 db 0			; creates 33 bytes after previous nop instruction to fill our boot record with arbitrary values

start:
	jmp 0x0:step2		; sets the code segment register to 0x0.  This will actually be address 0x7c00 since we set ORG 0x7c00

step2:
	cli			; clear interrupts (so that they're disabled while we change segment registers)
	mov ax, 0x0
	mov ds, ax		; setup data segment.  note that data segment will start at 0x7c0 * 16 + 0, or 0x7c00, which is the location that BIOS jumps to for execution of bootloader
	mov es, ax		; setup extra segment
	mov ss, ax
	mov sp, 0x7c00 		; remember from before: 0x7c00 is the address that RAM will jump to execute bootloader.  ss * 16 + sp = 0x7c00
	sti			; enables interrupts

	mov si, message 	; si stands for source index. 16 bit low end of esi register
	call print


.load_protected:		; load the processor into protected mode
	cli
	lgdt[gdt_descriptor]
	mov eax, cr0
	or al, 0x1		; set PE (Protection Enable) bit in CR0 (control register 0)
	mov cr0, eax
	jmp CODE_SEG:load32 	; sets the code segment selector to CODE_SEG and jumps to load32

; GDT
gdt_start:
gdt_null:
	dd 0x0 			; 4 bytes of 0s
	dd 0x0 			; This will represent a null entry (null segment descriptor) in our gdt

; offset 0x8 - this is the offset in our GDT where we will make the code segment descriptor entry (will be the 2nd entry in our gdt since each entry is 8 bytes)
gdt_code:			; CS SHOULD POINT TO THIS
	dw 0xffff 		; Segment Limit 0:15 bits.  This is the maximum addressable unit for this segment (page granularity, 4 KiB in this case)
	dw 0			; Base 0:15.  The base describes the linear address where the segment begins
	db 0			; Base 16:23 bits
	db 0x9a			; Access byte
	db 11001111b		; High 4 bit flags and low 4 bit flags
	db 0			; Base 24-31 bits

; offset 0x10			; (3rd entry in our gdt - we're actually using this for DS, SS, ES, FS, and GS since we're not really using segmentation anyways)
gdt_data:			; DS, SS, ES, FS, GS
	dw 0xffff 		; Segment Limit 0:15 bits.  This is the maximum addressable unit for this segment (page granularity, 4 KiB in this case)
	dw 0			; Base 0:15.  The base describes the linear address where the segment begins
	db 0			; Base 16:23 bits
	db 0x92			; Access byte
	db 11001111b		; High 4 bit flags and low 4 bit flags
	db 0			; Base 24-31 bits

gdt_end:

gdt_descriptor:			; This is the GDT description structure that will be used by the lgdt instruction
	dw gdt_end - gdt_start - 1	; Size
	dd gdt_start		; Offset - linear address of the GDT itself


print:
	mov bx, 0		; part of interrupt call
.loop:
	lodsb			; loads the character that si register is pointing to into al register.  then increments si register so it points to next char
	cmp al, 0
	je .done
	call print_char
	jmp .loop

.done:				; nasm sub label
	ret

print_char:
	mov ah, 0eh		; function number = 0Eh : Display character
	int 0x10        	; call INT 10h (0x10), BIOS video service 
	ret


message:
	db 'StinkOS is booting...', 0

[BITS 32]
load32:
	; in 32 bit mode here
	; the goal is to load our kernel into memory and jump to it
	mov eax, 1		; eax will contain the starting sector we want to load from (0 is boot sector)
	mov ecx, 100		; ecx will contain the total number of sectors we want to load
	mov edi, 0x0100000	; edi will contain the address we want to load these sectors in to (1 MB)
	call ata_lba_read	; ata_lba_read is the label that will talk with the drive and load the sectors into memory
	jmp CODE_SEG:0x0100000 	; jump to 1 MB which is the location where we've read the kernel sectors into memory

ata_lba_read:
	; ata/ide is a protocol for communicating with disk drives attached via the motherboard to CPU
	mov ebx, eax 		; backup the logical block address (lba) 
	; send the highest 8 bits of the lba to disk controller
	shr eax, 24		; shift the eax register 24 bits to the right so that eax contains highest 8 bits of the lba (shr does not wrap)
	or eax, 0xE0		; selects the master drive
	mov dx, 0x01F6		; 0x01F6 is the port that we're expected to write these 8 bits to
	out dx, al		; al contains the 8 high bits from earlier.  
				; out instruction copies the value from al to the I/O port specified by dx (copies the address and value to the I/O bus on the motherboard)
	; Finished sending the highest 8 bits of the lba

	; send the total number of sectors that we are reading to the hard disk controller
	mov eax, ecx
	mov dx, 0x01F2		; 0x01F2 --> set sectorcount
	out dx, al
	; finished sending the total sectors to read
	
	; send low bits of the lba
	mov eax, ebx		; restore the backup lba
	mov dx, 0x01F3		; 0x01F3 --> set LBAlo
	out dx, al

	; Send mid bits of the lba
	mov dx, 0x01F4		; 0x01F4 --> LBAmid
	mov eax, ebx 		
	shr eax, 8
	out dx, al

	; Send upper 16 bits of the lba
	mov dx, 0x1F5		; 0x01F5 --> LBAhi
	mov eax, ebx		
	shr eax, 16 
	out dx, al

	mov dx, 0x01F7		; 0x01F7 --> Command IO Port
	mov al, 0x20		; 0x0020 --> Read sector(s) command
	out dx, al

	; Read all sectors into memory
.next_sector:
	push ecx		; push ecx onto the stack to save for later (remember it has the total # of sectors we want to read)

; checking if we need to read
.try_again:
	mov dx, 0x01F7	
	in al, dx		; we read the ATA status register into al by reading from the command io port
	test al, 8		; check to see if the busy bit is set.  If set, the disk drive still has control of the command block registers (still reading)
	jz .try_again		; jumps back to try_again if the busy bit is set

	; we need to read 256 words (512 bytes, or one sector) at a time
 	mov ecx, 256
	mov dx, 0x01F0
	rep insw		; rep insw reads a word from port 0x01F0 and stores it into 0x0100000 (1 MB, specified by edi register)
	pop ecx			; restores the ecx that we saved via push earlier (contains count of sectors left to read
	loop .next_sector	; decrements ecx register (contains # registers to read) until it hits 0 and we've read all sectors
	ret
	

times 510-($ - $$) db 0 ; pad our code with 0s up to 510 bytes
dw 0xAA55		; Intel machines are little endian so this will be flipped to 0x55AA

