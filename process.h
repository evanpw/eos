// Process class
#pragma once
#include <stddef.h>
#include <unistd.h>

struct OpenFileDescription;
struct File;

static constexpr size_t RLIMIT_NOFILE = 256;

// Represents and maintains state for a single usermode process. A process may
// have multiple threads, but they all share the same address space.
struct Process {
    pid_t pid;
    OpenFileDescription* openFiles[RLIMIT_NOFILE] = {};

    int open(File& file);
    int close(int fd);
};
