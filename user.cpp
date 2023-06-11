#include "user.h"

#include <stdint.h>

// syscall calling convention: rax (syscall #), rdi, rsi, rdx, r10, r8

int64_t syscall(uint64_t function, uint64_t arg1 = 0, uint64_t arg2 = 0,
                uint64_t arg3 = 0, uint64_t arg4 = 0, uint64_t arg5 = 0) {
    int64_t result;

    register uint64_t r10 __asm__("r10") = arg4;
    register uint64_t r8 __asm__("r8") = arg5;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(function), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10),
                   "r"(r8)
                 : "rcx", "r11", "memory");

    return result;
}

void umain() {
    int64_t r1 = syscall(0, 1, 2);
    syscall(1, r1);
    int64_t r2 = syscall(0, 3, 4);
    syscall(1, r2);
    int64_t r3 = syscall(0, 5, 6);
    syscall(1, r3);
    int64_t r4 = syscall(0, r1, r2);
    syscall(1, r4);
    int64_t result = syscall(0, r4, r3);
    syscall(1, result);

    while (true)
        ;
}
