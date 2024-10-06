#pragma once
#include <stdint.h>

#include "estd/memory.h"
#include "estd/vector.h"
#include "spinlock.h"

struct Thread;

extern "C" uint64_t currentKernelStack;
extern "C" [[noreturn]] void enterContext(Thread* toThread);

// An opaque token whose identity represents a particular state that's blocking
// some thread
struct Blocker {
    Blocker() = default;

    // No copy / move
    Blocker(const Blocker&) = delete;
    Blocker& operator=(const Blocker&) = delete;
};

struct BlockedThread {
    Thread* thread;
    estd::shared_ptr<Blocker> blocker;
};

// This is the structure created on the stack by calling switchContext, which is used to
// save and restore the caller-saved registers and return address across a context switch
struct ThreadContext {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbx;
    uint64_t rbp;
    uint64_t returnAddress;
};

static_assert(sizeof(ThreadContext) == 7 * sizeof(uint64_t));

struct Scheduler {
public:
    Scheduler();
    void start();

    void startThread(Thread* thread);
    void threadExit();

    void sleepThread(const estd::shared_ptr<Blocker>& blocker, Spinlock* lock = nullptr);
    void wakeThreads(const estd::shared_ptr<Blocker>& blocker);
    void wakeThreadsLocked(const estd::shared_ptr<Blocker>& blocker);

    void onTimerInterrupt();

private:
    void yield();
    void cleanupDeadThreads();

    estd::vector<Thread*> runQueue;
    estd::vector<BlockedThread> waitQueue;
    estd::vector<Thread*> deadQueue;

    bool running = false;
    size_t nextIdx = 0;
    Spinlock _schedLock;

    estd::unique_ptr<Thread> _idleThread;
};
