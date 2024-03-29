[BITS 32]
section .asm

; This program prints a message. Then, it waits for a key to be pressed.
; When a key is pressed, it prints the key to the display.

; Three syscalls are tested: PRINT, GET_KEY_PRESS, PUT_CHAR_ON_DISPLAY

global _start
_start:

   ; Call the "print" kernel command
   push message
   mov eax, 1              ; Command 1 PRINT
   int 0x80
   add esp, 4              ; Restore the stack (remove pushed value)

_loop:
   call get_key
   push eax
   mov eax, 3              ; Command 1 PUT_CHAR_ON_DISPLAY
   int 0x80
   add esp, 4              ; Restore the stack (remove pushed value)

   jmp _loop


get_key:
   mov eax, 2              ; Command 2 GET_KEY_PRESS TODO: implement syscall
   int 0x80
   cmp eax, 0x00
   je get_key              ; If the value in eax is 0x00, we jump back to get_key.
                          ; This implies that the current process's keyboard buffer was empty.
   ret                    ; If the value in eax was not 0x00, then we retrieved a key from the buffer.

section .data
message: 
    db 'Hello from userland!', 0

key_press_message: 
    db 'You pressed a key!', 0