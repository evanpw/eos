// https://pubs.opengroup.org/onlinepubs/7908799/xsh/unistd.h.html
#pragma once
#include <sys/types.h>

ssize_t write(int fd, const void* buffer, size_t count);
ssize_t read(int fd, void* buffer, size_t count);
pid_t getpid();
