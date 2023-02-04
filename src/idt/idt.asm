section .asm

extern int21h_handler
extern int_generic_handler
extern isr80h_handler

global idt_load
global int21h_entry
global int_generic_entry
global enable_interrupts
global disable_interrupts
global isr80h_wrapper

enable_interrupts:
	sti 
	ret

disable_interrupts:
	cli 
	ret

idt_load:
	push ebp					; preserve the caller's frame pointer by pushing it onto the stack
	mov ebp, esp				; set the value of the current frame pointer to equal the stack pointer

	mov ebx, [ebp+8]			; ebp+8 points to the first argument that is passed into this function by the caller
	lidt [ebx]					; load the idtr

	pop ebp						; restore the caller's base pointer value by popping ebp off the stack
	ret							; return to the caller - ret finds and removes the appropriate return address from the stack

int21h_entry:					; keyboard interrupt handler (Interrupt vector #0x21)
	cli							; clear interrupt flag
	pushad						; Push EAX, ECX, EDX, EBX, original ESP, EBP, ESI, and EDI (all general purpose registers)
	call int21h_handler
	popad						; restore general purpose registers
	sti							; set interrupt flag (allow processor to respond to maskable hardware interrupts)
	iret						; interrupt return

int_generic_entry:				; for interrupts vectors that we haven't written handlers for yet
	cli							; clear interrupt flag
	pushad						; Push EAX, ECX, EDX, EBX, original ESP, EBP, ESI, and EDI (all general purpose registers)
	call int_generic_handler
	popad						; restore general purpose registers
	sti							; set interrupt flag (allow processor to respond to maskable hardware interrupts)
	iret						; interrupt return

isr80h_wrapper:
	; INTERRUPT FRAME START
	; ALREADY PUSHED TO US BY THE PROCESS UPON ENTRY TO THIS INTERRUPT
	; uint32_t ip
	; uint32_t cs
	; uint32_t flags
	; uint32_t sp
	; uint32_t ss
	pushad						; push all general purpose registers onto the stack (eax, ecd, edx, ebx, original esp, ebp, esi, and edi)
								; we want to save them to the stack because the kernel code needs to use them.
	; INTERRUPT FRAME END

	; Push the stack pointer so that we are pointing to interrupt frame + general purpose registers
	push esp
	push eax					; Push the command the userland passed us to the stack.  This determines the operation the kernel will perform.
	call isr80h_handler			; This is a C function. C functions put return values in eax.  Thus why we move eax value into tmp_res on next line.
	mov dword[tmp_res], eax		; Move the value in eax into the address that tmp_res points to. dword = 4 bytes.

	; Remove the two items that we added to the stack.  This way, esp points to the interrupt frame again.
	add esp, 8

	; Restore the general purpose registers for userland
	popad

	; Put the returned value from isr80h_handler back into eax, since popad will have overwritten it.
	; tmp_res was just a temporary place to store the returned value while we set the gp registers before switching back into userland.
	mov eax, [tmp_res]

	; use iretd to return from the interrupt.  It will restore ip, cs, flags, sp, and ss registers
	; from interrupt frame (which will drop us back into userland)
	iretd

section .data
; This is used to store the return result from isr80h_handler
tmp_res: dd 0



