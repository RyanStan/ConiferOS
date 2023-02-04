[BITS 32]
section .asm

; This is for testing userland functionality

global _start
_start:

   ; Call the "sum" kernel command
   mov eax, 0
   int 0x80

   jmp $                   ; Infinite loop
