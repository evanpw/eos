#include "fcntl.h"
#include "syscall.h"

int open(const char* path, int oflag) {
    return __syscall(SYS_open, (uint64_t)path, oflag);
}
