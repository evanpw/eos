#include "stdio.h"

#include "syscall.h"
#include "unistd.h"

int putchar(int c) {
    if (try_syscall(SYS_write, STDOUT_FILENO, &c, 1) < 0) {
        return EOF;
    }

    return c;
}
