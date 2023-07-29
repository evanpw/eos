#include "unistd.h"

#include <stdint.h>

#include "sys/types.h"
#include "syscall.h"

ssize_t read(int fd, void* buffer, size_t count) {
    return __syscall(SYS_read, fd, (uint64_t)buffer, count);
}

ssize_t write(int fd, const void* buffer, size_t count) {
    return __syscall(SYS_write, fd, (uint64_t)buffer, count);
}

pid_t getpid() { return __syscall(SYS_getpid); }

// Non-standard
int sleep(int ticks) { return __syscall(SYS_sleep, ticks); }
