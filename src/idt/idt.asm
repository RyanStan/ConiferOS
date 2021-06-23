section .asm

extern int21h_handler
extern int_generic_handler

global idt_load
global int21h_entry
global int_generic_entry

idt_load:
	push ebp			; preserve the caller's frame pointer by pushing it onto the stack
	mov ebp, esp			; set the value of the current frame pointer to equal the stack pointer

	mov ebx, [ebp+8]		; ebp+8 points to the first argument that is passed into this function by the caller
	lidt [ebx]			; load the idtr

	pop ebp				; restore the caller's base pointer value by popping ebp off the stack
	ret				; return to the caller - ret finds and removes the appropriate return address from the stack

int21h_entry:				; keyboard interrupt handler (Interrupt vector #0x21)
	cli				; clear interrupt flag
	pushad				; Push EAX, ECX, EDX, EBX, original ESP, EBP, ESI, and EDI (all general purpose registers)
	call int21h_handler
	popad				; restore general purpose registers
	sti				; set interrupt flag (allow processor to respond to maskable hardware interrupts)
	iret				; interrupt return

int_generic_entry:				; for interrupts vectors that we haven't written handlers for yet
	cli				; clear interrupt flag
	pushad				; Push EAX, ECX, EDX, EBX, original ESP, EBP, ESI, and EDI (all general purpose registers)
	call int_generic_handler
	popad				; restore general purpose registers
	sti				; set interrupt flag (allow processor to respond to maskable hardware interrupts)
	iret				; interrupt return

