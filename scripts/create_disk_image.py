import sys
import os
import struct

boot_filename = sys.argv[1]
kernel_filename = sys.argv[2]
diskimg_filename = sys.argv[3]

with open(boot_filename, "rb") as fh:
    boot_data = fh.read()

with open(kernel_filename, "rb") as fh:
    kernel_data = fh.read()

def pad_to_sector_size(data):
    remainder = len(data) % 512
    if remainder == 0:
        return data

    padding = 512 - remainder
    return data + bytearray(padding)

kernel_data = pad_to_sector_size(kernel_data)

assert len(boot_data) == 512
assert len(kernel_data) % 512 == 0

boot_offset = 0
boot_size = 1

diskmap_offset = 1
diskmap_size = 1

kernel_offset = boot_size + diskmap_size
kernel_size = len(kernel_data) // 512

diskmap_data = struct.pack("HH", kernel_offset, kernel_size)
diskmap_data = pad_to_sector_size(diskmap_data)
assert len(diskmap_data) == 512

with open(diskimg_filename, "wb") as fh:
    fh.write(boot_data)
    fh.write(diskmap_data)
    fh.write(kernel_data)
