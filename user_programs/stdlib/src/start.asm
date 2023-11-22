[BITS 32]

global _start
extern main

section .asm

_start:
    ; We are going to call the main function.
    ; We need to take argc and argv from the stack frame that the kernel
    ; created for this process, and pass them to the main function as arguments.
    
    ; Push argv (esp+8) onto the stack
    mov eax, [esp + 8]
    push eax

    ; Push argc (esp+4) onto the stack
    mov eax, [esp + 4]
    push eax

    call main

    ; Clean up the stack
    add esp, 8

    ; TODO [RyanStan 11-19-23] Implement an exit routine, and handle user program exits from the kernel
    ret