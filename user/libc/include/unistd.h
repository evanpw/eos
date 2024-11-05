// https://pubs.opengroup.org/onlinepubs/7908799/xsh/unistd.h.html
#pragma once
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

ssize_t write(int fd, const void* buffer, size_t count);
ssize_t read(int fd, void* buffer, size_t count);
pid_t getpid();
[[noreturn]] void _exit(int status);
void* sbrk(intptr_t incr);
int pipe(int filedes[2]);
pid_t fork();
int execvp(const char* path, const char* argv[]);
int isatty(int filedes);
int dup2(int filedes, int filedes2);

int chdir(const char* path);
char* getcwd(char* buffer, size_t size);

off_t lseek(int fd, off_t offset, int whence);

// Non-standard
int sleep(int ticks);
pid_t launch(const char* path, const char* argv[]);

#ifdef __cplusplus
}
#endif
