mkdir -p build

# Boot loader
nasm -f elf64 -g -F dwarf boot.asm -o build/boot.o
$HOME/opt/cross/bin/x86_64-elf-ld -Ttext=0x7C00 -melf_x86_64 -e0x7c00 build/boot.o -o build/boot.elf
$HOME/opt/cross/bin/x86_64-elf-objcopy -O binary build/boot.elf build/boot.bin

# Kernel
$HOME/opt/cross/bin/x86_64-elf-gcc -g -ffreestanding -mno-red-zone -O -c kernel.c -o build/kernel.o
$HOME/opt/cross/bin/x86_64-elf-gcc -Ttext=0x7E00 -e0x7E00 -nostdlib -lgcc build/kernel.o -o build/kernel.elf
$HOME/opt/cross/bin/x86_64-elf-objcopy -O binary build/kernel.elf build/kernel.bin

# Disk image
cat build/boot.bin build/kernel.bin > build/diskimg
