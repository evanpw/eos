// Process class
#pragma once
#include <stddef.h>
#include <unistd.h>

#include "estd/ownptr.h"
#include "file.h"
#include "page_map.h"
#include "thread.h"

static constexpr int RLIMIT_NOFILE = 256;

// Represents and maintains state for a single usermode process. A process may
// have multiple threads, but they all share the same address space.
struct Process {
    Process(const char* filename);
    ~Process();

    pid_t pid;
    OwnPtr<OpenFileDescription> openFiles[RLIMIT_NOFILE] = {};
    OwnPtr<UserAddressSpace> addressSpace;
    OwnPtr<Thread> thread;

    int open(File& file);
    int close(int fd);

    static pid_t s_nextPid;
};
