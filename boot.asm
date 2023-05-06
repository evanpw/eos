; vim: syntax=nasm
BITS 16
ORG 0x7C00

main:
    ; Disable interrupts
    cli

    ; Make sure we're at 0x0000:0x7C00 instead of 0x07C0:0x0000
    jmp 0x0000:.setcs

.setcs:
    ; For paging we need an empty buffer of 16KiB (4 levels of tables, each 4KiB long)
    ; Place it at es:di = 0x0000:0x9000
    xor ax, ax
    mov es, ax
    mov edi, 0x9000
    mov ecx, 0x1000 ; 0x1000 dwords
    xor eax, eax
    cld
    rep stosd

    ; Page Map Level 4
    mov edi, 0x9000
    lea eax, [es:di + 0x1000] ; eax = pointer to the page directory pointer table
    or eax, 3 ; PAGE_PRESENT | PAGE_WRITE
    mov [es:di], eax

    ; Page Directory Pointer Table
    lea eax, [es:di + 0x2000] ; eax = pointer to the page directory
    or eax, 3 ; PAGE_PRESENT | PAGE_WRITE
    mov [es:di + 0x1000], eax

    ; Page Directory
    lea eax, [es:di + 0x3000] ; eax = pointer to the page table
    or eax, 3 ; PAGE_PRESENT | PAGE_WRITE
    mov [es:di + 0x2000], eax

    ; Page Table
    lea di, [di + 0x3000] ; di = pointer to the page table
    mov eax, 3 ; address 0 + PAGE_PRESENT | PAGE_WRITE

.pageTableLoop:
    ; Identity map
    mov [es:di], eax

    ; Move to the next page in memory and the next page table entry
    add eax, 0x1000
    add di, 8

    ; Loop for the first 2MiB of memory
    cmp eax, 0x200000
    jb .pageTableLoop

    ; Set PAE (Physical Address Extension) and PGE (Page Global Enabled) flags
    mov eax, 10100000b
    mov cr4, eax

    ; Point cr3 to the PML4 to set up paging
    mov edx, 0x9000
    mov cr3, edx

    ; Set the LME (long mode enabled) bit of the EFER (extended feature enable register) MSR (model-specific register)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x00000100
    wrmsr

    ; Load the GDT (global descriptor table)
    xor ax, ax
    mov ds, ax
    lgdt [GDT.pointer]

    ; Activate long mode by enabling paging and entering protected mode at the same time
    mov ebx, cr0
    or ebx, 0x80000001
    mov cr0, ebx

    ; Far jump to set cs to the code selector (and clear the instruction pipeline)
    jmp GDT.code:.set64

BITS 64
.set64:
    ; Zero out data-segment registers not used in 64-bit mode
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Set up a stack
    mov ax, GDT.data
    mov ss, ax

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
        times 5 db 0
        db 10011000b        ; present, ring 0, non-system, executable, non-conforming
        db 00100000b        ; long mode
        db 0

    .data: equ $ - GDT
        times 5 db 0
        db 10010010b        ; present, ring 0, non-system, data, writeable
        times 2 db 0

    .pointer:
        dw $ - GDT - 1
        dd GDT

; Boot sector must be 512 bytes
times 510 - ($ - $$) db 0

; Boot sector must end with this magic number
db 0x55
db 0xAA
