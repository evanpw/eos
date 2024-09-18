#pragma once
#include <stdint.h>

#include "estd/ownptr.h"
#include "estd/sharedptr.h"
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
    SharedPtr<Blocker> blocker;
};

struct Scheduler {
public:
    Scheduler();
    void start();

    void startThread(Thread* thread);
    void stopThread(Thread* thread);

    void sleepThread(const SharedPtr<Blocker>& blocker, Spinlock* lock = nullptr);
    void wakeThreads(const SharedPtr<Blocker>& blocker);

    void onTimerInterrupt();

private:
    void yield();
    void cleanupDeadThreads();

    Vector<Thread*> runQueue;
    Vector<BlockedThread> waitQueue;
    Vector<Thread*> deadQueue;

    bool running = false;
    size_t nextIdx = 0;
    Spinlock _schedLock;

    OwnPtr<Thread> _idleThread;
};
