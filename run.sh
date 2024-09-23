qemu-system-x86_64 \
    -drive file=build/diskimg,index=0,if=ide,format=raw \
    -debugcon stdio \
    -D log.txt \
    -m 5G \
    --no-reboot \
    -cpu qemu64,pdpe1gb \
    -netdev user,id=net0,hostfwd=tcp::10022-:22 \
    -device e1000,netdev=net0 \
    -object filter-dump,id=f1,netdev=net0,file=network.pcap \
    $@
