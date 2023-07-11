[BITS 32]
section .asm

; This is for testing userland functionality

global _start
_start:

   ; Call the "sum" kernel command
   push 20
   push 30
   mov eax, 0              ; Command 0 SUM
   int 0x80
   add esp, 8              ; Restore the stack (remove pushed values)

   jmp $                   ; Infinite loop
