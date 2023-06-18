#include "syscall.h"

int64_t __syscall(uint64_t function, uint64_t arg1, uint64_t arg2,
                  uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    int64_t result;

    register uint64_t r10 __asm__("r10") = arg4;
    register uint64_t r8 __asm__("r8") = arg5;
    register uint64_t r9 __asm__("r9") = arg6;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(function), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10),
                   "r"(r8), "r"(r9)
                 : "rcx", "r11", "memory");

    return result;
}
