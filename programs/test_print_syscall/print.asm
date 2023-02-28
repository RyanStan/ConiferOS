[BITS 32]
section .asm

global _start
_start:

   ; Call the "print" kernel command
   push message
   mov eax, 1              ; Command 1 PRINT
   int 0x80
   add esp, 4              ; Restore the stack (remove pushed value)

   jmp $                   ; Infinite loop

section .data
message: 
    db 'Hello from userland!', 0