// Thread class
#pragma once

#include "address.h"
#include "estd/ownptr.h"

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

    static OwnPtr<Thread> createUserThread(Process* process, VirtualAddress entryPoint);
    static OwnPtr<Thread> createKernelThread(VirtualAddress entryPoint);

    Process* process;

    // Keep track of allocations so they can be freed
    PhysicalAddress userStackPages;
    PhysicalAddress kernelStackPages;

    ~Thread();
};

static_assert(offsetof(Thread, kernelStack) == 0);
static_assert(offsetof(Thread, rsp) == 8);

extern "C" Thread* currentThread;
