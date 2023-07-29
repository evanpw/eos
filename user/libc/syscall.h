#pragma once
#include <stdint.h>

#include "api/syscalls.h"

int64_t __syscall(uint64_t function, uint64_t arg1 = 0, uint64_t arg2 = 0,
                  uint64_t arg3 = 0, uint64_t arg4 = 0, uint64_t arg5 = 0,
                  uint64_t arg6 = 0);
