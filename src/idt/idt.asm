section .asm

extern isr80h_handler
extern interrupt_handler

global idt_load
global enable_interrupts
global disable_interrupts
global isr80h_wrapper
global interrupt_pointer_table

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

; Create a generic interrupt wrapper that will take an interrupt number as an argument
; and pass that argument to the C function which will call the correct C interrupt handler.
%macro interrupt 1
	global int%1
	int%1:
		; INTERRUPT FRAME START
		; ALREADY PUSHED TO US BY THE PROCESS UPON ENTRY TO THIS INTERRUPT
		; uint32_t ip
		; uint32_t cs
		; uint32_t flags
		; uint32_t sp
		; uint32_t ss
		cli							; clear interrupt flag
		pushad						; push all general purpose registers onto the stack (eax, ecd, edx, ebx, original esp, ebp, esi, and edi)
									; so that we can restore these values to prevent the "interrupted code" from losing its state.
		; INTERRUPT FRAME END
		push esp 					; The esp captured with pushad is the esp before the pushad operation.
				 					; However, we want to capture the value of esp after the pushad operation, 
				 					; so that this esp references to the stack location after all the general purpose registers.
		push dword %1				; Push the interrupt number to the stack so that interrupt_handler can access it as an argument
		call interrupt_handler
		add esp, 8					; Remove the two items that we added to the stack.
									; This way, esp points to the interrupt frame again. Recall that the stack grows downwards towards lower memory addresses.
		popad						; Restore all the general purpose registers
		sti							; set interrupt flag (allow processor to respond to maskable hardware interrupts)
		iret						; Return from the interrupt
%endmacro

; Instantiate our interrupt code for interrupts 0 - 512. 
; This allows us to have symbols for int0 - int512. Each will call interrupt handler and pass it the corresponding interrupt number.
; Since the addresses of these interrupt handlers won't be known at compilation time, we must create the interrupt_pointer_table array in the
; data section. This array will contain the addresses of the interrupt handlers so that they can be dynamically accessed from idt.c.
%assign i 0
%rep 512
	interrupt i
%assign i i+1
%endrep

; We aren't going to use the interrupt macro for this, because isr80h_wrapper receives an argument from userspace (the system call #)
isr80h_wrapper:
	; INTERRUPT FRAME START
	; ALREADY PUSHED TO US BY THE PROCESS UPON ENTRY TO THIS INTERRUPT
	; uint32_t ip
	; uint32_t cs
	; uint32_t flags
	; uint32_t sp
	; uint32_t ss
	pushad						; push all general purpose registers onto the stack (eax, ecd, edx, ebx, original esp, ebp, esi, and edi)
								; so that we can restore them before dropping back into userland. This allows our kernel code to use these registers
								; without worrying about losing the state of the user space application that called this interrupt.
	cli							; clear interrupt flag
	; INTERRUPT FRAME END

	push esp 					; The esp captured with pushad is the esp before the pushad operation.
								; However, we want to capture the value of esp after the pushad operation, 
								; so that this esp references to the stack location after all the general purpose registers.
	push eax
	call isr80h_handler			; This is a C function. C functions put return values in eax.  Thus why we move eax value into tmp_res on next line.
	mov dword[tmp_res], eax		; Move the value in eax into the address that tmp_res points to. dword = 4 bytes.

	; Remove the two items that we added to the stack.  This way, esp points to the interrupt frame again.
	; Recall that the stack grows downwards towards lower memory addresses.
	add esp, 8

	; Restore the general purpose registers for userland
	popad

	; Put the returned value from isr80h_handler back into eax, since popad will have overwritten it.
	; tmp_res was just a temporary place to store the returned value while we set the gp registers before switching back into userland.
	mov eax, [tmp_res]

	sti							; set interrupt flag (allow processor to respond to maskable hardware interrupts)

	; use iretd to return from the interrupt.  It will restore ip, cs, flags, sp, and ss registers
	; from interrupt frame (which will drop us back into userland)
	iretd

section .data
; This is used to store the return result from isr80h_handler
tmp_res: dd 0

; This macro is used to get the address of an interrupt handler based on it's label.
; e.g. if the input is 5, then 'dd int5' gives us the address of the int5 label. 
; dd = 4 bytes = size of address on 32 bit architecture (x86)
%macro interrupt_array_entry 1
	dd int%1
%endmacro

; Since the addresses of our interrupt handlers won't be known at compilation time (due to weirdness of nasm macros), we must create the interrupt_pointer_table array in the
; data section. This array will contain the addresses of the interrupt handlers so that they can be dynamically accessed from idt.c.
interrupt_pointer_table:
%assign i 0
%rep 512
	interrupt_array_entry i
%assign i i+1
%endrep
