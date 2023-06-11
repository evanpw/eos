import os
import sys
import math

SECTOR_SIZE = 512

size = 0
for filename in sys.argv[:-1]:
    size += os.path.getsize(filename)

size_in_sectors = int(math.ceil(size // SECTOR_SIZE))

with open(sys.argv[-1], "w") as fh:
    print(f"KERNEL_SIZE_IN_SECTORS equ {size_in_sectors}", file=fh)
