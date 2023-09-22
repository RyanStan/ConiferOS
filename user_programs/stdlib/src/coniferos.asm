[BITS 32]

section .asm

; Assembly interface with Kernel system calls

global print:function
global get_key:function
global coniferos_malloc:function
global coniferos_free:function

; void print(const char *filename)
print:
    push ebp
    mov ebp, esp
    push dword[ebp+8]               ; ebp+8 will point to the argument (char *filename), since we pushed 8 byte ebp to the stack earlier
                                    ; We push that value onto the stack
    mov eax, 1                      ; Print system call
    int 0x80                        ; Interrupt 0x80 is used for system calls
    add esp, 4                      ; Restore the stack (stack shrinks by incrementing esp)
    pop ebp
    ret

; int get_key()
get_key:
    push ebp
    mov ebp, esp
    mov eax, 2                      ; Get key press system call
    int 0x80                        ; This system call puts return value in eax, which is what C expects (as defined by ABI, I think)
    pop ebp
    ret

; void *coniferos_malloc(size_t size)
coniferos_malloc:
    push ebp
    mov ebp, esp
    mov eax, 4                      ; Malloc system call
    push dword[ebp+8]               ; Pushes the size variable to the stack.
                                    ; This assumes that size_t is dword size (4 bytes) on the build host architecture.
    int 0x80
    add esp, 4
    pop ebp
    ret

; void coniferos_free(void *ptr)
coniferos_free:
    push ebp
    mov ebp, esp
    mov eax, 5                      ; free system call
    push dword[ebp+8]               ; Pushes the ptr variable to the stack.
    int 0x80
    add esp, 4
    pop ebp
    ret