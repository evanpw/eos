// Process class
#pragma once
#include <stddef.h>
#include <unistd.h>

#include "estd/ownptr.h"
#include "estd/vector.h"
#include "file.h"
#include "page_map.h"
#include "thread.h"

static constexpr int RLIMIT_NOFILE = 256;

// Singleton class which owns all of the processes
class ProcessTable {
    friend class Process;

public:
    static void init();
    static ProcessTable& the() {
        ASSERT(_instance);
        return *_instance;
    }

private:
    static ProcessTable* _instance;

    Process* create(const char* filename);
    void destroy(Process* process);

    Vector<OwnPtr<Process>> _processes;
    pid_t _nextPid = 1;
};

// Represents and maintains state for a single usermode process. A process may
// have multiple threads, but they all share the same address space.
class Process {
    friend class ProcessTable;

public:
    ~Process();

    // Actually performed by ProcessTable, but access from Process for clarity
    static Process* create(const char* filename) {
        return ProcessTable::the().create(filename);
    }
    static void destroy(Process* process) { ProcessTable::the().destroy(process); }

    pid_t pid;
    OwnPtr<OpenFileDescription> openFiles[RLIMIT_NOFILE] = {};
    OwnPtr<UserAddressSpace> addressSpace;
    OwnPtr<Thread> thread;
    PhysicalAddress imagePages;

    int open(const SharedPtr<File>& file);
    int close(int fd);

private:
    Process(pid_t pid, const char* filename);
};
