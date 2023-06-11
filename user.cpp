#include "user.h"

#include <stdint.h>

// syscall calling convention: rax (syscall #), rdi, rsi, rdx, r10, r8, r9

int64_t syscall(uint64_t function, uint64_t arg1 = 0, uint64_t arg2 = 0,
                uint64_t arg3 = 0, uint64_t arg4 = 0, uint64_t arg5 = 0,
                uint64_t arg6 = 0) {
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

extern "C" void umain() {
    int64_t result = syscall(2, 1, 2, 3, 4, 5, 6);
    syscall(1, result);

    while (true)
        ;
}
