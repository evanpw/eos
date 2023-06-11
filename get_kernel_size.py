import os
import sys
import math

SECTOR_SIZE = 512
size = os.path.getsize(sys.argv[1])
size_in_sectors = int(math.ceil(size // SECTOR_SIZE))

with open(sys.argv[2], "w") as fh:
    print(f"KERNEL_SIZE_IN_SECTORS equ {size_in_sectors}", file=fh)
