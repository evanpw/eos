import sys
import os
import struct
import subprocess
import shutil

boot_bin = sys.argv[1]
boot_elf = sys.argv[2]
kernel_bin = sys.argv[3]
output_filename = sys.argv[4]

def capture(cmd):
    return subprocess.run(
        cmd,
        capture_output=True,
        shell=True,
        check=True,
        text=True
    ).stdout

def runcmd(cmd):
    return subprocess.run(
        cmd,
        shell=True,
        check=True,
    )

# Create a 32 MiB zeroed-out disk image
with open(output_filename, "wb") as fh:
    fh.write(bytearray(32 * 1024 * 1024))

# Mount it as a loopback device so we can touch individual partitions
loop_filename = capture(f"sudo losetup --find --partscan --show {output_filename}").strip()

try:
    # Create a single ext2 partition, with 1MiB of empty space at the beginning of the
    # image where we'll put the boot loader and kernel
    runcmd(f'sudo parted -s "{loop_filename}" mklabel msdos mkpart primary ext2 1MiB 100% -a minimal set 1 boot on')

    # Create the ext2 filesystem in the just-created partition
    runcmd(f"sudo mke2fs {loop_filename}p1")

finally:
    runcmd(f"sudo losetup -d {loop_filename}")


with open(boot_bin, "rb") as fh:
    boot_data = bytearray(fh.read())

with open(kernel_bin, "rb") as fh:
    kernel_data = fh.read()

with open(output_filename, "rb") as fh:
    mbr_data = fh.read(512)

def pad_to_sector_size(data):
    remainder = len(data) % 512
    if remainder == 0:
        return data

    padding = 512 - remainder
    return data + bytearray(padding)


kernel_data = pad_to_sector_size(kernel_data)

assert len(boot_data) == 512
assert len(kernel_data) % 512 == 0
assert len(boot_data) + len(kernel_data) < 1024 * 1024

# Find the offset in the bootloader which holds the offset and size of the kernel
dap_offset = None
dap_size = None
symbols = capture(f"nm {boot_elf}")
for line in symbols.splitlines():
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

# Fill in the partition table from the original MBR sector
boot_data[0x1B8:0x200] = mbr_data[0x1B8:0x200]

with open(output_filename, "r+b") as fh:
    fh.seek(0)
    fh.write(boot_data)
    fh.write(kernel_data)
