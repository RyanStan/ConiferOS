section .asm
global tss_load

tss_load:
    push ebp		    ; preserve caller's frame pointer
    mov ebp, esp		; create new frame pointer pointing to current stack top
    mov ax, [ebp + 8]   ; Get TSS segment selector from arguments and store in ax
    ltr ax              ; Load TSS segment selector into segment selector field of task register
    pop ebp             ; set ebp to caller's frame pointer
    ret