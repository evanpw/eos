#include "sys/ioctl.h"

#include "syscall.h"

int ioctl(int fd, int op, void* argp) { return try_syscall(SYS_ioctl, fd, op, argp); }
