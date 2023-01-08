[BITS 32]		; all code below here is seen as 32-bit code.  The brackets indicate a primitive directive
section .asm

global task_enter_userland
global user_registers

; void restore_general_purpose_registers(struct registers *registers)
; Load the general purpose registers with their respective values from the passed in registers structure 
restore_general_purpose_registers:
    push ebp            ; preserve caller's frame pointer
    mov ebp, esp        ; create new frame pointer pointing to current stack top
    mov ebx, [ebp+8]    ; Load the registers parameter (an address) into the ebx register so we can pull the rest of the registers
    mov edi, [ebx]
    mov esi, [ebx+4]
    mov ebp, [ebx+8]
    mov edx, [ebx+16]
    mov ecx, [ebx+20]
    mov eax, [ebx+24]
    mov ebx, [ebx+12]   ; Now we can properly set ebx since we've set the other registers
    add esp, 4          ; Restore ebp to caller's frame pointer.  However, we don't want to overwrite ebp register so we don't use pop.
    ret

; void task_enter_userland(struct registers *registers)
task_enter_userland:

    ; Normally, when a processor enters an interrupt handler (via 'int' instruction), it pushes eip, cs, eflags, esp, ss
    ; to the stack.  Then when it exits, it calls iretd to return to where it was.  We're taking advantage of the iret
    ; instruction here to simulate returning from an interrupt as a means of entering user mode.  This is necessary
    ; because we can only set these registers via call instructions (like iretd).  More details on what the iretd instruction
    ; does further down.

    mov ebp, esp        ; create new frame pointer pointing to current stack top
    mov ebx, [ebp+4]    ; Load the registers parameter (an address) into the ebx register so we can pull the rest of the registers
                        ; Normally, we use + 8 to access the first argument.  However, since at the beginning of this function, we didn't push
                        ; the caller's frame pointer to the stack with 'push ebp', the first argument starts at the 4th byte and not the 8th.

    ; dword is four bytes (size of gp register as well)

    ; Push the data/stack selector (ss)
    push dword [ebx+44]

    ; Push the stack pointer (esp)
    push dword [ebx+40]

    ; Push eflags (and set interrupt enable flag)
    pushf               ; pushf is a fancy instruction that pushes eflags register onto the stack
    pop eax             ; pop the flags into the eax register
    or eax, 0x200       ; set interrupt enable flag (allow processor to recognize external interrupts on the INTR pin)
    push eax            ; push flags back onto stack
    ; TODO We're pushing the kernel eflags here... we should push the register eflags since that will be the user land eflags


    ; Push the code segment
    push dword [ebx+32]

    ; Push instruction pointer register (ip)
    push dword [ebx+28]

    ; Set segment register which won't be set by iret instruction
    ; For now, we just set them all to the same value as the stack segment selector
    mov ax, [ebx+44]
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Restore all registers to what was passed in the registers struct
    push dword [ebp+4]
    call restore_general_purpose_registers
    add esp, 4                  ; We need to take the top element back off the stack.  We can't pop it, because then that
                                ; would alter the contents of one of the registers we're trying to set.  So we just modify the stack pointer.

    ; Leave kernel land and execute in user land
    ; 
    ; IRET instruction pops the return instruction pointer, return code
    ; segment selector, and EFLAGS image from the stack to the EIP, CS, and EFLAGS registers, respectively, and then
    ; resumes execution of the interrupted program or procedure. If the return is to another privilege level, the IRET
    ; instruction also pops the stack pointer and SS from the stack, before resuming program execution. If the return is
    ; to virtual-8086 mode, the processor also pops the data segment registers from the stack
    ;
    ; eip
    ; cs
    ; eflags
    ; esp
    ; ss

    iretd

user_registers:
    mov ax, 0x23                ; 0x20 is the correct offset into our gdt for the user data segment. 
                                ; However, the RPL is stored in the low two bits of the segment selector.
                                ; Hence, by setting segment selectors to 0x23, the RPL will be 3.
                                ; Source: http://www.brokenthorn.com/Resources/OSDev23.html
                                ; TODO: do we really need to set RPL to 3 though? Paging model removed need for RPL
                                ; as far as I know.
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret

