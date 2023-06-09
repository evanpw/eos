set -e
mkdir -p build

# Kernel
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -mgeneral-regs-only -O -c kmain.cpp -o build/kmain.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -mgeneral-regs-only -O -c screen.cpp -o build/screen.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -mgeneral-regs-only -O -c assertions.cpp -o build/assertions.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -mgeneral-regs-only -O -c print.cpp -o build/print.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -mgeneral-regs-only -O -c mem.cpp -o build/mem.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -mgeneral-regs-only -O -c stdlib.cpp -o build/stdlib.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -mgeneral-regs-only -O -c new.cpp -o build/new.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -mgeneral-regs-only -O -c system.cpp -o build/system.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -mgeneral-regs-only -O -c interrupts.cpp -o build/interrupts.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -mgeneral-regs-only -O -c user.cpp -o build/user.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -mgeneral-regs-only -O -c keyboard.cpp -o build/keyboard.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -mgeneral-regs-only -O -c terminal.cpp -o build/terminal.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -mgeneral-regs-only -O -c page_map.cpp -o build/page_map.o
$HOME/opt/cross/bin/x86_64-elf-g++ -Ttext=0x7E00 -ekmain -nostdlib -lgcc -o build/kernel.elf \
    build/kmain.o \
    build/screen.o \
    build/assertions.o \
    build/print.o \
    build/mem.o \
    build/stdlib.o \
    build/new.o \
    build/interrupts.o \
    build/user.o \
    build/keyboard.o \
    build/system.o \
    build/terminal.o \
    build/page_map.o
$HOME/opt/cross/bin/x86_64-elf-objcopy -O binary build/kernel.elf build/kernel.bin --set-section-flags .bss=alloc,load,contents

# Boot loader
nasm -f elf64 -g -F dwarf boot.asm -DKERNEL_SIZE_IN_SECTORS=$(python get_kernel_size.py) -o build/boot.o
$HOME/opt/cross/bin/x86_64-elf-ld -Ttext=0x7C00 -melf_x86_64 -e0x7c00 build/boot.o -o build/boot.elf
$HOME/opt/cross/bin/x86_64-elf-objcopy -O binary build/boot.elf build/boot.bin

# Disk image
cat build/boot.bin build/kernel.bin > build/diskimg
