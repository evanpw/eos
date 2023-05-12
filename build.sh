mkdir -p build

# Boot loader
nasm -f elf64 -g -F dwarf boot.asm -o build/boot.o
$HOME/opt/cross/bin/x86_64-elf-ld -Ttext=0x7C00 -melf_x86_64 -e0x7c00 build/boot.o -o build/boot.elf
$HOME/opt/cross/bin/x86_64-elf-objcopy -O binary build/boot.elf build/boot.bin

# Kernel
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -O -c kmain.cpp -o build/kmain.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-exceptions -O -c video.cpp -o build/video.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-exceptions -O -c misc.cpp -o build/misc.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-exceptions -O -c cstring.cpp -o build/cstring.o
$HOME/opt/cross/bin/x86_64-elf-g++ -Ttext=0x7E00 -ekmain -nostdlib -lgcc build/kmain.o build/video.o build/misc.o build/cstring.o -o build/kernel.elf
$HOME/opt/cross/bin/x86_64-elf-objcopy -O binary build/kernel.elf build/kernel.bin

# Disk image
cat build/boot.bin build/kernel.bin > build/diskimg
