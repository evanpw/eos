#include "fcntl.h"
#include "errno.h"

#include "syscall.h"

int open(const char* path, int oflag) {
    int result = __syscall(SYS_open, (uint64_t)path, oflag);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return result;
}
