; Intel syntax
; I know, there are a lot of comments.  I'm a newbie to Intel syntax and x86 assembly 

ORG 0x7c00				; This is where the program expects to be loaded into memory
BITS 16					; Tells the assembler we are using a 16 bit architecture

CODE_SEG equ gdt_code - gdt_start	; EQU is a NASM psuedo instruction that gives a symbol (CODE_SEG) a corresponding value (gdt_code - gdt_start)
DATA_SEG equ gdt_data - gdt_start	; These symbols will be used to give us our offsets into the GDT for the respective segment descriptors

		
jmp short start				; Start of FAT boot sector
nop

; FAT16 Header
OEMIdentifier:		db 'CONIFER '
BytesPerSector:		dw 0x0200	; Generally ignored by most kernels
SectorsPerCluster:	db 0x80		; decimal 128
ReservedSectors: dw 0x0FA1		; Our kernel will be stored in the reserved sectors (4001 sectors, or about 2.04 MB)
FATCopies:		db 0x02		; Number of file allocation tables on the file system
RootDirEntries:		dw 0x0040	; Root directory must occupy entire sectors (decimal 64).  This value contain the number of possible (max) entries in the root directory. Its recommended that the number of entries is an even multiple of the BytesPerSector values. The recommended value for FAT16 volumes is 512 entries (compatibility reasons).
NumSectors:		dw 0x0000	; Not using this
MediaType:		db 0xF8		; Fixed disk media type
SectorsPerFAT:		dw 0x0100	; # of sectors per file allocation table (decimal 256)
SectorsPerTrack:	dw 0x0020	; cylinder-head-sector (chs) addressing
NumberOfHeads:		dw 0x0040	; # of heads on the storage media (chs)
HiddenSectors:		dd 0x00000000	; Number of hidden sectors preceding this partition. (i.e. the LBA of the beginning of the partition.)
SectorsBig:		dd 0x00773594	; Large sector count. This field is set if there are more than 65535 sectors in the volume, resulting in a value which does not fit in the Sectors Per Cluster field

; Extended BPB (DOS 4.0)
DriveNumber:		db 0x80
WinNTBit:		db 0x00
Signature:		db 0x29
VolumeID:		dd 0xD105	
VolumeIDString:		db 'CONIFEROS  '
SystemIDString:		db 'FAT16   '

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
	db 'ConiferOS is booting...', 0

[BITS 32]
load32:
	; in 32 bit mode here
	; the goal is to load our kernel into memory and jump to it
	mov eax, 1		; eax will contain the starting sector we want to load from (0 is boot sector)
	mov ecx, 4000		; ecx will contain the total number of sectors we want to load (2.04 MB MB)
	mov edi, 0x0100000	; edi will contain the address we want to load these sectors in to (1 MB)
	call ata_lba_read	; ata_lba_read is the label that will talk with the drive and load the sectors into memory
	jmp CODE_SEG:0x0100000 	; jump to 1 MB which is the location where we've read the kernel sectors into memory


; https://wiki.osdev.org/ATA_read/write_sectors
;=============================================================================
; ATA read sectors (LBA mode) 
;
; ata/ide is a protocol for communicating with disk drives attached via the motherboard to CPU
; Primary channel ports
; Data Port: 0x1F0
; Error/Features Port: 0x1F1
; Sector Count: 0x1F2
; LBA/Sector Number: 0x1F3-0x1F5
; Drive/Head Select: 0x1F6
; Status/Command: 0x1F7
; Alternate Status: 0x3F610
;
; Common ATA commands
; 0x20: Read Sectors (PIO mode)
; 0xEC: Identify Drive
; 0xA0: ATAPI Packet Command
; 0x30: Write Sectors (PIO mode)
;
; @param EAX Logical Block Address of sector to start read from
; @param ECX number of sectors to read
; @param EDI Address of buffer to put data obtained from disk
;
; @return None
; 
; Side effect: this will increment EAX.
;=============================================================================
ata_lba_read:
	push ecx				; Preserve ecx argument
.read_loop:
	; If ecx is 0, then there are 0 more sector to read.
	test ecx, ecx
	jz .done

	cmp ecx, 255
	jl .read_remaining 	; If ECX <= 255, read remaining sectors

	push ecx				; Save ECX in case callee modifies it.
	mov ecx, 255				; Read 255 sectors
	call ata_lba_read_inner
	pop ecx					; Restore sector count

	sub ecx, 255			; Subtract 255 from sectors to read count
	add edi, 512*255		; Update the output buffer address (512 bytes per sector * 255 sectors)
	add eax, 255			; Increment LBA by 255.
	jmp .read_loop

.read_remaining:
	call ata_lba_read_inner

.done
	pop ecx
	ret

; https://wiki.osdev.org/ATA_read/write_sectors
;=============================================================================
; ATA read sectors (LBA mode) 
;
;
; @param EAX Logical Block Address of sector to start read from
; @param CL number of sectors to read (lower 8 bits of ecx register)
; @param EDI Address of buffer to put data obtained from disk
;
; @return None
; 
; Side effect: this will increment EAX.
;=============================================================================
ata_lba_read_inner:
	mov ebx, eax 		; backup the logical block address (lba) 

	; send the highest 8 bits of the lba to disk controller
	shr eax, 24			; shift the eax register 24 bits to the right so that eax contains highest 8 bits of the lba (shr does not wrap)
	or eax, 0xE0		; selects the master drive
	mov dx, 0x01F6		; 0x01F6 is the port that we're expected to write these 8 bits to
	out dx, al			; al is implicitly set as the lower 8 bits of the eax register. Therefore, this is the high 8 bits of the lba 
						; out instruction copies the value from al to the I/O port specified by dx (copies the address and value to the I/O bus on the motherboard)


	; send the total number of sectors that we are reading to the hard disk controller
	; (max 255 per transaction because the sector count register on the disk only has 8 bits).
	mov al, cl	; We have to move cl into al because the out instruction only uses the EAX register.

.send_sector_count:
	mov dx, 0x01F2		; 0x01F2 --> sectorcount port
	out dx, al
	
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

	mov dx, 0x01F7		; 0x01F7 --> Command IO Port (TODO: stuck here?)
	mov al, 0x20		; 0x0020 --> Read sector(s) command
	out dx, al

	; Read sectors into memory
.read_loop:
	push ecx		; Save remaining sector count.

; Wait for the disk drive to be ready.
.wait_for_ready_drive:
	mov dx, 0x01F7	
	in al, dx				; we read the ATA status register into al by reading from the command io port
	test al, 8				; check to see if the busy bit is set.  If set, the disk drive still has control of the command block registers (still reading)
	jz .wait_for_ready_drive		; jumps back to try_again if the busy bit is set

	; Read one sector (512 bytes, or 256 words) to the address at EDI
 	mov ecx, 256
	mov dx, 0x01F0	; data port
	rep insw		; rep insw reads a word from port 0x01F0 and stores it into 0x0100000 (1 MB, specified by edi register)
					; insw reads from the io port specified by dx into the memory location specified by edi.
					; The instruction will increment or decrement edi as necessary. 
					; The rep instruction repeats the insw instruction n times, where n is the value in the ecx register
	pop ecx			; Restore remaing sector count (255) so that we can decrement it.
	loop .read_loop	; decrements ecx register (contains # registers to read) until it hits 0 and we've read all sectors
	ret

times 510-($ - $$) db 0 ; pad our code with 0s up to 510 bytes
dw 0xAA55		; Intel machines are little endian so this will be flipped to 0x55AA

