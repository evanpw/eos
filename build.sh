nasm -f elf64 -g -F dwarf boot.asm -o boot.o
$HOME/opt/cross/bin/x86_64-elf-ld -Ttext=0x7c00 -melf_x86_64 -e0x7c00  boot.o -o boot.elf
$HOME/opt/cross/bin/x86_64-elf-objcopy -O binary boot.elf boot.bin
