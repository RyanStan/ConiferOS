[BITS 32]

section .asm

; Assembly interface with Kernel system calls

global print:function
global coniferos_get_key:function
global coniferos_malloc:function
global coniferos_free:function
global coniferos_putchar:function
global coniferos_exec:function
global coniferos_execve:function

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

; int coniferos_get_key()
coniferos_get_key:
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
    mov ebp, esp                    ; Move the stack pointer (points to top of stack) into the base pointer (points to current stack frame)
    mov eax, 5                      ; free system call
    push dword[ebp+8]               ; Pushes the ptr variable to the stack.
    int 0x80
    add esp, 4
    pop ebp
    ret

; void coniferos_putchar(char c)
coniferos_putchar:
    push ebp
    mov ebp, esp
    mov eax, 3                      ; put char system call
    push dword[ebp+8]               ; push the character argument to the stack
    int 0x80
    add esp, 4
    pop ebp
    ret                             ; Debugging story time: I forget to add this 'ret' statement but gdb saved the day. 
                                    ; I used the 'run_no_restart' make target to force qemu to print the state of the registers + the exception.
                                    ; It was a page fault. The EIP was 0x001060ee. I started the debug server with 'make debug'.
                                    ; Then I executed my 'kernel debug' vscode configuration which attaches to the remote gdb server, and attached a breakpoint in GDB with break *0x001060ee' 
                                    ; After hitting the break point, I ran 'disassemble' which made it clear that the error was occurring here.
                                    ; 
                                    ; Thought I'd write this down to save it somewhere, in case I come back to this project years from now and need
                                    ; to remember how to debug issues.
                                    ; Maybe I should add it to the readme instead, under a debug section...

; void coniferos_exec(const char *filename)
coniferos_exec:
    push ebp
    mov ebp, esp
    mov eax, 6                      ; exec system call
    push dword[ebp+8]               ; push the 'const char *filename' arg onto the stack
    int 0x80
    add esp, 4
    pop ebp
    ret

; void coniferos_execve(const char *filename, const char *argv[], const int argc)
coniferos_execve:
    push ebp
    mov ebp, esp
    mov eax, 6                      ; exec system call
    push dword[ebp+8]               ; Push filename onto the stack
    push dword[ebp+12]              ; Push argv onto the stack
    push dword[ebp+16]              ; Push argc onto the stack 
    int 0x80
    add esp, 12
    pop ebp
    ret