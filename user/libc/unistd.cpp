#include "unistd.h"

#include <stdint.h>

#include "errno.h"
#include "sys/types.h"
#include "syscall.h"

ssize_t read(int fd, void* buffer, size_t count) {
    int64_t result = __syscall(SYS_read, fd, (uint64_t)buffer, count);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return result;
}

ssize_t write(int fd, const void* buffer, size_t count) {
    int64_t result = __syscall(SYS_write, fd, (uint64_t)buffer, count);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return result;
}

pid_t getpid() { return __syscall(SYS_getpid); }

void _exit(int status) {
    __syscall(SYS_exit, status);
    __builtin_unreachable();
}

void* sbrk(intptr_t incr) {
    int64_t result = __syscall(SYS_sbrk, incr);

    if (result < 0) {
        errno = -result;
        return (void*)-1;
    }

    return (void*)result;
}

// Non-standard
int sleep(int ticks) { return __syscall(SYS_sleep, ticks); }
void launch(const char* filename) { __syscall(SYS_launch, (uint64_t)filename); }
