import subprocess
import tempfile


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


loop_filename = capture(f"losetup --find --partscan --show build/diskimg").strip()
tmpdir = tempfile.mkdtemp()
runcmd(f"mount {loop_filename}p1 {tmpdir}")

print(f"Mounted disk image at {tmpdir}")
print("Run this command to clean up:")
print(f"sudo sh -c 'umount {tmpdir}; losetup -d {loop_filename}'")
