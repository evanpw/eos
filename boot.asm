; Tell the assembler that this will be loaded at address 0x7C00
[org 0x7C00]
    mov ax, 0x12AB
    call printReg

    ; Hang forever
    jmp $


    ; Routine which prints the string pointed to by ds:si to the screen
print:
    cld

ch_loop:
    ; Load one byte from ds:si into al
    lodsb

    ; When the null terminator is reached, exit the loop
    or al, al
    jz done

    ; Call the BIOS to print one character
    mov ah, 0x0E
    mov bh, 0
    int 0x10

    ; Continue to the next character
    jmp ch_loop

done:
    ret


    ; Routine which prints the value of the register ax
printReg:
    ; Move the value into dx so we can use ax
    mov dx, ax

    ; Point si to the hex-character lookup table
    mov si, hexstr

    ; 4 nibbles in a word
    mov cx, 4

hexloop:
    ; Iterate from most to least significant nibble
    rol dx, 4

    ; Mask out the lower 4 bits
    mov bx, dx
    and bx, 0x0F

    ; Convert to a character by table lookup
    mov al, [si + bx]

    ; Call the BIOS to print one character
    mov ah, 0x0E
    mov bh, 0
    int 0x10

    ; Continue the loop for all 4 nibbles
    loop hexloop

    ret


    ; Data
msg     db 'Hello World', 13, 10, 0
hexstr  db '0123456789ABCDEF'

    ; Boot sector must be 512 bytes
    times 510-($-$$) db 0

    ; Boot sector must end with this magic number
    db 0x55
    db 0xAA
