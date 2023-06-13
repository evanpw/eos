#include "user.h"

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

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

ssize_t write(int fd, const void* buffer, size_t count) {
    return syscall(4, fd, (uint64_t)buffer, count);
}

ssize_t read(int fd, void* buffer, size_t count) {
    return syscall(3, fd, (uint64_t)buffer, count);
}

extern "C" void umain() {
    const char* msg = "Hello World!\n";
    write(1, msg, 13);

    char buffer[64];
    while (true) {
        while (read(0, buffer, 64) == 0) {
        }

        if (buffer[0] == 'y') {
            write(1, "\byes\n", 5);
        } else {
            write(1, "\bno\n", 4);
        }
    }

    while (true)
        ;
}
