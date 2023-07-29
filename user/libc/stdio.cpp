#include "stdio.h"

#include "syscall.h"
#include "unistd.h"

int putchar(int c) {
    if (__syscall(SYS_write, STDOUT_FILENO, (uint64_t)&c, 1) != 1) {
        return EOF;
    }

    return c;
}
