; vim: syntax=nasm
BITS 16
ORG 0x7C00

main:
    ; Disable interrupts
    cli

    ; Make sure we're at 0x0000:0x7C00 instead of 0x07C0:0x0000
    jmp 0x0000:.setcs

.setcs:
    ; Load gdt
    xor ax, ax
    mov ds, ax
    lgdt [GDT.pointer]

    ; Enter protected mode by setting lower bit of cr0
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump to set cs to the code selector (and clear the instruction pipeline)
    jmp GDT.code:.set32

BITS 32
.set32:
    ; Set data segment registers to data selector (offset into gdt)
    mov ax, GDT.data
    mov ds, ax
    mov ss, ax
    mov es, ax

    ; Enable a20 gate to allow addresses above 1mb
    mov al, 2
    out 0x92, al

    ; Clear the screen
    cld
    mov edi, 0xB8000
    mov ecx, 80 * 25
    xor eax, eax
    rep stosw

    ; Print "OK"
    mov byte [0xB8000], 'O'
    mov byte [0xB8001], 0x0F
    mov byte [0xB8002], 'K'
    mov byte [0xB8003], 0x0F

    ; Hang forever
    jmp $

; Global descriptor table
GDT:
    .null: equ $ - GDT
        dq 0

    .code: equ $ - GDT
        dw 0xFFFF           ; size of 4gb
        db 0, 0, 0          ; base address of 0 (lower 3 bytes)
        db 10011010b        ; present, ring 0, not system, code, non-conforming, readable, not accessed
        db 11001111b        ; granularity 4kb, 32-bit default address size, reserved=0, available=0, top nibble of size
        db 0                ; upper byte of base address

    .data: equ $ - GDT
        dw 0xFFFF           ; size of 4gb
        db 0, 0, 0          ; base address of 0 (lower 3 bytes)
        db 10010010b        ; present, ring 0, not system, data, non-conforming, writeable, not accessed
        db 11001111b        ; granularity 4kb, 32-bit default address size, reserved=0, available=0, top nibble of size
        db 0                ; upper byte of base address

    .pointer:
        dw $ - GDT - 1
        dd GDT

; Boot sector must be 512 bytes
times 510-($-$$) db 0

; Boot sector must end with this magic number
db 0x55
db 0xAA
