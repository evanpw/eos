#include "stdlib.h"

#include "syscall.h"

void exit(int status) {
    __syscall(SYS_exit, status);
    __builtin_unreachable();
}
