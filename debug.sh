qemu-system-x86_64 -drive file=boot.bin,index=0,if=ide,format=raw -s -S&
x86_64-elf-gdb boot.elf -ex 'target remote localhost:1234' -ex 'layout src' -ex 'layout regs' -ex 'break stage2' -ex 'continue'
