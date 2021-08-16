[BITS 32]

section .asm

global paging_load_pgd
global enable_paging

paging_load_pgd:
        push ebp                        ; save the caller's base pointer
        mov ebp, esp                    ; set up this function's stack frame by setting it's frame pointer to the current stack bottom (stack grows down)

        mov eax, [ebp+8]                ; store the argument (address of the pgd to load into cr3) that was passed to this function call
        mov cr3, eax

        pop ebp			        ; set ebp to caller's frame pointer value
	ret			        ; return control to caller

enable_paging:
        push ebp                        ; save the caller's base pointer
        mov ebp, esp                    ; set up this function's stack frame by setting it's frame pointer to the current stack bottom (stack grows down)

        mov eax, cr0                    ; can't directly alter the value in cr0, so we must load it temporarily load it into eax
        or eax, 0x80000000              ; set the paging bit in cr0 so that paging is enabled
        mov cr0, eax

        pop ebp			        ; set ebp to caller's frame pointer value
	ret			        ; return control to caller