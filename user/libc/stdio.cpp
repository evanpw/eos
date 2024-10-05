#include "stdio.h"

#include "syscall.h"
#include "unistd.h"

int putchar(int c) {
    if (try_syscall(SYS_write, STDOUT_FILENO, &c, 1) < 0) {
        return EOF;
    }

    return c;
}

int puts(const char* s) {
    int ret = 0;
    while (*s) {
        if (putchar(*s++) == EOF) {
            return EOF;
        }
        ret++;
    }

    if (putchar('\n') == EOF) {
        return EOF;
    }

    return ret + 1;
}
