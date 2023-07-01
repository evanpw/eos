// Process class
#pragma once
#include <stddef.h>
#include <unistd.h>

#include "estd/ownptr.h"

struct OpenFileDescription;
struct File;

static constexpr int RLIMIT_NOFILE = 256;

// Represents and maintains state for a single usermode process. A process may
// have multiple threads, but they all share the same address space.
struct Process {
    pid_t pid;
    OwnPtr<OpenFileDescription> openFiles[RLIMIT_NOFILE] = {};

    int open(File& file);
    int close(int fd);
};
