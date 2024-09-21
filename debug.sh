qemu-system-x86_64 -drive file=build/diskimg,index=0,if=ide,format=raw -m 5G --no-reboot -cpu qemu64,pdpe1gb -s -S&
$HOME/opt/cross/bin/x86_64-elf-gdb build/kernel.elf -ex 'target remote localhost:1234' -ex 'set confirm off' -ex 'add-symbol-file build/user/shell.elf 0x8000000000' -ex 'layout src' -ex 'layout regs' -ex 'break *0x7E00' -ex 'continue'
wait
