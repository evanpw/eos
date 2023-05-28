set -e
mkdir -p build

# Kernel
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -O -c kmain.cpp -o build/kmain.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -O -c video.cpp -o build/video.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -O -c assertions.cpp -o build/assertions.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -O -c print.cpp -o build/print.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -O -c mem.cpp -o build/mem.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -O -c stdlib.cpp -o build/stdlib.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -mgeneral-regs-only -O -c interrupts.cpp -o build/interrupts.o
$HOME/opt/cross/bin/x86_64-elf-g++ -g -ffreestanding -mno-red-zone -fno-rtti -fno-exceptions -mgeneral-regs-only -O -c user.cpp -o build/user.o
$HOME/opt/cross/bin/x86_64-elf-g++ -Ttext=0x7E00 -ekmain -nostdlib -lgcc build/kmain.o build/video.o build/assertions.o build/print.o build/mem.o build/stdlib.o build/interrupts.o build/user.o -o build/kernel.elf
$HOME/opt/cross/bin/x86_64-elf-objcopy -O binary build/kernel.elf build/kernel.bin --set-section-flags .bss=alloc,load,contents

# Boot loader
nasm -f elf64 -g -F dwarf boot.asm -DKERNEL_SIZE_IN_SECTORS=$(python get_kernel_size.py) -o build/boot.o
$HOME/opt/cross/bin/x86_64-elf-ld -Ttext=0x7C00 -melf_x86_64 -e0x7c00 build/boot.o -o build/boot.elf
$HOME/opt/cross/bin/x86_64-elf-objcopy -O binary build/boot.elf build/boot.bin

# Disk image
cat build/boot.bin build/kernel.bin > build/diskimg
