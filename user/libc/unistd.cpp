#include "unistd.h"

#include <stdint.h>

#include "sys/types.h"
#include "syscall.h"

ssize_t read(int fd, void* buffer, size_t count) {
    return try_syscall(SYS_read, fd, buffer, count);
}

ssize_t write(int fd, const void* buffer, size_t count) {
    return try_syscall(SYS_write, fd, buffer, count);
}

pid_t getpid() { return syscall(SYS_getpid); }

void _exit(int status) {
    syscall(SYS_exit, status);
    __builtin_unreachable();
}

void* sbrk(intptr_t incr) { return try_syscall<void*>(SYS_sbrk, incr); }

int pipe(int filedes[2]) { return try_syscall(SYS_pipe, filedes); }

int chdir(const char* path) { return try_syscall(SYS_chdir, path); }

char* getcwd(char* buffer, size_t size) {
    if (try_syscall(SYS_getcwd, buffer, size) < 0) {
        return nullptr;
    }

    return buffer;
}

pid_t fork() { return try_syscall(SYS_fork); }

int execvp(const char* path, const char* argv[]) {
    return try_syscall(SYS_execvp, path, argv);
}

int isatty(int filedes) { return try_syscall(SYS_isatty, filedes); }

int dup2(int filedes, int filedes2) { return try_syscall(SYS_dup2, filedes, filedes2); }

off_t lseek(int fd, off_t offset, int whence) {
    return try_syscall(SYS_lseek, fd, offset, whence);
}

// Non-standard
int sleep(int ticks) { return syscall(SYS_sleep, ticks); }

pid_t launch(const char* path, const char* argv[]) {
    return syscall(SYS_launch, path, argv);
}
