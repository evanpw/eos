// Thread class
#pragma once

#include "address.h"
#include "estd/memory.h"

class Process;

struct Thread {
    // Always points to the top of the kernel stack. When this thread is running in user
    // mode, the kernel stack is empty, and on syscall entry the stack pointer is set to
    // this value. The entry code assumes that this value is at offset 0 within the Thread
    // struct.
    uint64_t kernelStack;

    // For not currently-running threads, this stores the current kernel stack pointer,
    // which is used by switchContext to restore the thread context and resume execution
    uint64_t rsp;

    static estd::unique_ptr<Thread> createUserThread(Process* process,
                                                     VirtualAddress entryPoint,
                                                     const char* programName,
                                                     const char* argv[]);
    static estd::unique_ptr<Thread> createKernelThread(VirtualAddress entryPoint);

    Process* process;

    //// User stack
    PhysicalAddress userStackTop;
    VirtualAddress userStackTopVirt;
    size_t userStackPages;

    PhysicalAddress userStackBottom() const {
        return userStackTop - userStackPages * PAGE_SIZE;
    }

    VirtualAddress userStackBottomVirt() const {
        return userStackTopVirt - userStackPages * PAGE_SIZE;
    }

    //// Kernel stack
    PhysicalAddress kernelStackTop;
    size_t kernelStackPages;

    PhysicalAddress kernelStackBottom() const {
        return kernelStackTop - kernelStackPages * PAGE_SIZE;
    }

    ~Thread();
};

static_assert(offsetof(Thread, kernelStack) == 0);
static_assert(offsetof(Thread, rsp) == 8);

extern "C" Thread* currentThread;
