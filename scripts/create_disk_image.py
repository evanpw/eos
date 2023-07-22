import atexit
import os
import shutil
import struct
import subprocess
import sys
import tempfile

build_dir = sys.argv[1]
output_filename = f"{build_dir}/diskimg"


def capture(cmd):
    return subprocess.run(
        cmd, capture_output=True, shell=True, check=True, text=True
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

# Change ownership to the non-root user which called this script
uid = int(os.environ["SUDO_UID"])
gid = int(os.environ["SUDO_GID"])
shutil.chown(output_filename, user=uid, group=gid)

# Mount it as a loopback device so we can touch individual partitions
loop_filename = capture(f"losetup --find --partscan --show {output_filename}").strip()

# Create a single ext2 partition, with 1MiB of empty space at the beginning of the
# image where we'll put the boot loader and kernel
runcmd(
    f'parted -s "{loop_filename}" mklabel msdos mkpart primary ext2 1MiB 100% -a minimal set 1 boot on'
)

# Create the ext2 filesystem in the just-created partition
runcmd(f"mke2fs {loop_filename}p1")

atexit.register(lambda: runcmd(f"losetup -d {loop_filename}"))

with open(f"{build_dir}/boot.bin", "rb") as fh:
    boot_data = bytearray(fh.read())

with open(f"{build_dir}/kernel.bin", "rb") as fh:
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
symbols = capture(f"nm {build_dir}/boot.elf")
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

with open(loop_filename, "r+b") as fh:
    fh.seek(0)
    fh.write(boot_data)
    fh.write(kernel_data)

# Mount the disk image and copy the user files to it
with tempfile.TemporaryDirectory() as tmpdir:
    runcmd(f"mount {loop_filename}p1 {tmpdir}")

    try:
        shutil.copy(f"{build_dir}/shell.bin", tmpdir)
    finally:
        runcmd(f"umount {tmpdir}")
