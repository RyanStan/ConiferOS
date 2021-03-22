; Intel syntax
; I know, there are a lot of comments.  I'm a newbie to Intel syntax and x86 assembly 

ORG 0x7c00	; This is odd, I don't think it's actually needed
BITS 16		; Tells the assembler we are using a 16 bit architecture

start:
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


