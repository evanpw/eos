import os
import sys
import math

SECTOR_SIZE = 512

filename = sys.argv[1]
size = os.path.getsize(filename)

padding_needed = 512 - size % 512 + 512
with open(filename, "ab") as fh:
    fh.write(b"\0" * padding_needed)
