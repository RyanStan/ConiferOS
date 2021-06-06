section .asm

global idt_load

idt_load:
	push ebp			; preserve the caller's frame pointer by pushing it onto the stack
	mov ebp, esp			; set the value of the current frame pointer to equal the stack pointer

	mov ebx, [ebp+8]		; ebp+8 points to the first argument that is passed into this function by the caller
	lidt [ebx]			; load the idtr

	pop ebp				; restore the caller's base pointer value by popping ebp off the stack
	ret				; return to the caller - ret finds and removes the appropriate return address from the stack
