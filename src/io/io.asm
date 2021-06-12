section .asm

global insb
global insw
global outb
global outw

insb:
	push ebp		; preserve caller's frame pointer
	mov ebp, esp		; create new frame pointer pointing to current stack top

	xor eax, eax		; 0 out the eax register
	mov edx, [ebp+8]	; transfer port # into edx register
	in al, dx		; input byte from io port in dx into al
				; note: al is part of eax register, which is tradionally where return values go
	

	pop ebp			; set ebp to caller's frame pointer value
	ret			; return control to caller

insw:
	push ebp		; preserve caller's frame pointer
	mov ebp, esp		; create new frame pointer pointing to current stack top

	xor eax, eax		; 0 out the eax register
	mov edx, [ebp+8]	; transfer port # into edx register
	in ax, dx		; input word from io port in dx into ax
	

	pop ebp			; set ebp to caller's frame pointer value
	ret
	
outb:
	push ebp		; preserve caller's frame pointer
	mov ebp, esp		; create new frame pointer pointing to current stack top

	mov eax, [ebp+12]	; get val parameter
	mov edx, [ebp+8]	; get port parameter
	out dx, al		; output byte in al to io/port address in dx

	pop ebp			; set ebp to caller's frame pointer value
	ret

outw:
	push ebp		; preserve caller's frame pointer
	mov ebp, esp		; create new frame pointer pointing to current stack top

	mov eax, [ebp+12]	; get val parameter
	mov edx, [ebp+8]	; get port parameter
	out dx, ax		; output word in ax to io/port address in dx

	pop ebp			; set ebp to caller's frame pointer value
	ret

