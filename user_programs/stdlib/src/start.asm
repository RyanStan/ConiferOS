[BITS 32]

global _start
extern main

section .asm

_start:
    ; We are going to call the main function.
    ; When the kernel initializes a process, it loads argc into [esp]
    ; and argv into [esp + 4]. Here, we take these values and pass them
    ; to the main function as arguments.
    
    ; push argc onto the stack
    mov eax, [esp]
    push eax ; This will decrement esp, so next access to argv will be [esp + 8] instead of [esp + 4]

    ; push argv onto the stack
    mov eax, [esp + 8]
    push eax

    call main

    ; Clean up the stack
    add esp, 8

    ; TODO [RyanStan 11-19-23] Implement an exit routine, and handle user program exits from the kernel
    ret