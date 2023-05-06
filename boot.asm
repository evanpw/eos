[bits 16]
[org 0x7C00]
    ; Disable interrupts
    cli

    ; Load gdt
    xor ax, ax
    mov ds, ax
    lgdt [gdt_desc]

    ; Enter protected mode by setting lower bit of cr0
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump to set cs to the code selector
    jmp 0x08:stage2

[bits 32]
stage2:
    ; Set data segment registers to data selector (offset into gdt)
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax

    ; Create a stack in free memory above our code
    mov esp, 0x90000

    ; Print something to the screen by writing to video RAM
    mov byte [0xB8000], 'P'
    mov byte [0xB8001], 0x1B

    ; Enable a20 gate to allow addresses above 1mb
    mov al, 2
    out 0x92, al

    ; Hang forever
    jmp $


    ; Global descriptor table
gdt:
gdt_null:
    dq 0

gdt_code:
    dw 0xFFFF           ; size of 4gb
    db 0, 0, 0          ; base address of 0 (lower 3 bytes)
    db 0b10011010    ; present, ring 0, system, code, non-conforming, readable, not accessed
    db 0b11001111   ; granularity 4kb, 32-bit default address size, reserved=0, available=0, top nibble of size
    db 0                ; upper byte of base address

gdt_data:
    dw 0xFFFF           ; size of 4gb
    db 0, 0, 0          ; base address of 0 (lower 3 bytes)
    db 0b10010010    ; present, ring 0, system, data, non-conforming, writeable, not accessed
    db 0b11001111   ; granularity 4kb, 32-bit default address size, reserved=0, available=0, top nibble of size
    db 0                ; upper byte of base address
gdt_end:

    ; GDT descriptor
gdt_desc:
    dw gdt_end - gdt - 1
    dd gdt


    ; Boot sector must be 512 bytes
    times 510-($-$$) db 0

    ; Boot sector must end with this magic number
    db 0x55
    db 0xAA
