section .asm
global gdt_load

; https://stackoverflow.com/questions/74175454/x86-why-does-c-function-disassembly-show-different-value-for-pointer-when-pushed
; Still resolving above issue

gdt_load:
    mov eax, [esp + 4]              ; Load the first arg (pointer to first gdt entry) into eax register
    mov [gdt_descriptor + 2], eax   ; Load the address of first gdt entry into the descriptor's start address
    mov ax, [esp + 8]               ; Move the second argument (int size of gdt) into ax register. TODO: make sure this works
    mov [gdt_descriptor], ax        ; Fill gdt_descriptor with size of table
    lgdt [gdt_descriptor]           ; Load the gdt
    ret

section .data
gdt_descriptor:
    dw 0x00                         ; Size of table in bytes - 1 (dw = 2 bytes)
    dd 0x00                         ; GDT Start Address (dd = 4 bytes)
