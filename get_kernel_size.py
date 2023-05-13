import os
import math

SECTOR_SIZE = 512
size = os.path.getsize("build/kernel.bin")
size_in_sectors = int(math.ceil(size // SECTOR_SIZE))
print(size_in_sectors)
