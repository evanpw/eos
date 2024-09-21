// Process class
#pragma once
#include <stddef.h>
#include <unistd.h>

#include "estd/memory.h"
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

    Process* create(const char* path, const char* argv[]);
    void destroy(Process* process);

    estd::vector<estd::unique_ptr<Process>> _processes;
    pid_t _nextPid = 1;
};

// Represents and maintains state for a single usermode process. A process may
// have multiple threads, but they all share the same address space.
class Process {
    friend class ProcessTable;

public:
    ~Process();

    // Actually performed by ProcessTable, but access from Process for clarity
    static Process* create(const char* path, const char* argv[]) {
        return ProcessTable::the().create(path, argv);
    }
    static void destroy(Process* process) { ProcessTable::the().destroy(process); }

    VirtualAddress heapStart() const {
        return addressSpace->userMapBase() + imagePagesCount * PAGE_SIZE;
    }

    size_t heapSize() const { return heapPagesCount * PAGE_SIZE; }

    void createHeap(size_t size);

    pid_t pid;
    estd::unique_ptr<OpenFileDescription> openFiles[RLIMIT_NOFILE] = {};
    uint32_t cwdIno;

    estd::unique_ptr<UserAddressSpace> addressSpace;
    estd::unique_ptr<Thread> thread;

    // TODO: more flexible handling of process memory
    PhysicalAddress imagePages;
    uint64_t imagePagesCount;

    PhysicalAddress heapPages = 0;
    uint64_t heapPagesCount = 0;

    int open(const estd::shared_ptr<File>& file);
    int close(int fd);

private:
    Process(pid_t pid, const char* path, const char* argv[]);
};
