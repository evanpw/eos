qemu-system-x86_64 -drive file=build/diskimg,index=0,if=ide,format=raw -debugcon stdio -m 5G --no-reboot -cpu qemu64,pdpe1gb
