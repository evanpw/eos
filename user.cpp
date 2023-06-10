#include "user.h"

#include "screen.h"

int64_t syscall(uint64_t function, uint64_t arg0, uint64_t arg1, uint64_t arg2,
                uint64_t arg3, uint64_t arg4) {
    int64_t result;

    register uint64_t r10 __asm__("r10") = arg3;
    register uint64_t r8 __asm__("r8") = arg4;
    asm volatile(
        "syscall"
        : "=a"(result)
        // syscall calling convention: rax (syscall #), rdi, rsi, rdx, r10, r8
        : "a"(function), "D"(arg0), "S"(arg1), "d"(arg2), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory");

    return result;
}

void userTask() {
    int64_t result = syscall(100, 1, 2, 3, 4, 5);

    while (true)
        ;
}
