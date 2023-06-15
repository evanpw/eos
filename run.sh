qemu-system-x86_64 -drive file=build/diskimg,index=0,if=none,format=raw,id=disk -device piix4-ide -device ide-hd,drive=disk -debugcon stdio -m 5G --no-reboot
