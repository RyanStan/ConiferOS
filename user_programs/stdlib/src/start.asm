[BITS 32]

global _start
extern main

section .asm

_start:
    ; We are going to call the main function.
    ; When the kernel initializes a process, it loads argc into [esp]
    ; and argv into [esp + 4]. Here, we take these values and pass them
    ; to the main function as arguments.
    
    ; Push argv (esp+8) onto the stack
    mov eax, [esp + 4]
    push eax

    ; Push argc (esp+4) onto the stack
    mov eax, [esp]
    push eax

    call main

    ; Clean up the stack
    add esp, 8

    ; TODO [RyanStan 11-19-23] Implement an exit routine, and handle user program exits from the kernel
    ret