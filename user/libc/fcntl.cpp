#include "fcntl.h"

#include "syscall.h"

int open(const char* path, int oflag) { return try_syscall(SYS_open, path, oflag); }
int close(int fd) { return try_syscall(SYS_close, fd); }
