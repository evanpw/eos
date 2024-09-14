// https://pubs.opengroup.org/onlinepubs/7908799/xsh/unistd.h.html
#pragma once
#include <stddef.h>
#include <sys/types.h>

extern "C" {

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

ssize_t write(int fd, const void* buffer, size_t count);
ssize_t read(int fd, void* buffer, size_t count);
pid_t getpid();
[[noreturn]] void _exit(int status);

// Non-standard
int sleep(int ticks);
void launch(const char* path);
}
