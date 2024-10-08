// https://pubs.opengroup.org/onlinepubs/7908799/xsh/unistd.h.html
#pragma once
#include <fcntl.h>
#include <stddef.h>
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

int chdir(const char* path);
char* getcwd(char* buffer, size_t size);

// Non-standard
int sleep(int ticks);
pid_t launch(const char* path, const char* argv[]);

#ifdef __cplusplus
}
#endif
