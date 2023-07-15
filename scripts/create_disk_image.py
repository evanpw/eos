import sys
import os
import struct
import subprocess

boot_bin = sys.argv[1]
boot_elf = sys.argv[2]
kernel_bin = sys.argv[3]
out_filename = sys.argv[4]

with open(boot_bin, "rb") as fh:
    boot_data = bytearray(fh.read())

with open(kernel_bin, "rb") as fh:
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

# Find the offset in the bootloader which holds the offset and size of the kernel
dap_offset = None
dap_size = None
symbols = subprocess.run(
    f"nm {boot_elf}", capture_output=True, shell=True, check=True, text=True
)
for line in symbols.stdout.splitlines():
    address, kind, name = line.split()
    if name == "dap.offset":
        dap_offset = int(address, base=16) - 0x7C00
    elif name == "dap.size":
        dap_size = int(address, base=16) - 0x7C00

assert dap_offset is not None and dap_size is not None

# Replace those values in the image before writing
kernel_offset = 1
kernel_size = len(kernel_data) // 512

boot_data[dap_offset : dap_offset + 2] = struct.pack("H", kernel_offset)
boot_data[dap_size : dap_size + 2] = struct.pack("H", kernel_size)

with open(out_filename, "wb") as fh:
    fh.write(boot_data)
    fh.write(kernel_data)
