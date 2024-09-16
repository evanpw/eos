#include "scheduler.h"

#include "panic.h"
#include "process.h"
#include "processor.h"
#include "thread.h"

uint64_t currentKernelStack;

extern "C" void enterContextImpl(Thread* toThread) {
    currentThread = toThread;
    currentKernelStack = toThread->kernelStack;
    Processor::loadCR3(toThread->process->addressSpace->pml4());
    // TODO: save / restore FPU state
}

extern "C" [[noreturn]] void __attribute__((naked)) enterContext(Thread* /*toThread*/) {
    asm volatile(
        "mov 0x8(%%rdi),%%rsp\n"  // toThread->rsp
        "call enterContextImpl\n"
        "pop %%r15\n"
        "pop %%r14\n"
        "pop %%r13\n"
        "pop %%r12\n"
        "pop %%rbx\n"
        "pop %%rbp\n"
        "ret\n"
        :
        :
        : "memory");
}

void __attribute__((naked)) switchContext(Thread* /*toThread*/, Thread* /*fromThread*/) {
    asm volatile(
        "push %%rbp\n"
        "mov %%rsp, %%rbp\n"
        "push %%rbx\n"
        "push %%r12\n"
        "push %%r13\n"
        "push %%r14\n"
        "push %%r15\n"
        "mov %%rsp, 0x8(%%rsi)\n"  // fromThread->rsp
        "jmp enterContext\n"
        :
        :
        : "memory");
}

void Scheduler::start() {
    ASSERT(!running && !runQueue.empty());

    running = true;
    Thread* initialThread = runQueue[nextIdx];
    nextIdx = (nextIdx + 1) % runQueue.size();
    enterContext(initialThread);
}

void Scheduler::onTimerInterrupt() {
    SpinlockLocker locker(_schedLock);
    yield();
}

void Scheduler::yield() {
    ASSERT(_schedLock.isLocked());
    if (!running || runQueue.empty()) return;

    cleanupDeadThreads();

    Thread* fromThread = currentThread;
    Thread* toThread = runQueue[nextIdx];
    nextIdx = (nextIdx + 1) % runQueue.size();

    // We have to temporarily unlock the sched lock, or we'll deadlock on the next
    // timer interrupt, and then relock it before returning so that the caller can
    // continue safely
    {
        SpinlockUnlocker unlocker(_schedLock);
        switchContext(toThread, fromThread);
    }
}

void Scheduler::startThread(Thread* thread) {
    SpinlockLocker locker(_schedLock);
    runQueue.push_back(thread);
}

void Scheduler::stopThread(Thread* thread) {
    SpinlockLocker locker(_schedLock);

    for (size_t i = 0; i < runQueue.size(); ++i) {
        if (runQueue[i] != thread) continue;

        // Move the thread from the run queue to the dead queue
        Thread* lastThread = runQueue.back();
        runQueue[i] = lastThread;
        runQueue.pop_back();
        deadQueue.push_back(thread);

        ASSERT(!runQueue.empty());
        nextIdx = (i + 1) % runQueue.size();

        // If we're stopping our own thread, we need to switch to something else
        if (thread == currentThread) {
            yield();
        } else {
            return;
        }
    }

    panic("Thread not found");
}

void Scheduler::cleanupDeadThreads() {
    ASSERT(_schedLock.isLocked());

    bool foundSelf = false;
    for (Thread* thread : deadQueue) {
        // We can't clean up a thread until we've switched away from it
        if (thread == currentThread) {
            foundSelf = true;
            continue;
        }

        Process::destroy(thread->process);
    }

    deadQueue.clear();

    if (foundSelf) {
        deadQueue.push_back(currentThread);
    }
}

void Scheduler::sleepThread(const SharedPtr<Blocker>& blocker, Spinlock& lock) {
    SpinlockLocker locker(_schedLock);

    // We can't hold this lock while sleeping
    lock.unlock();

    for (size_t i = 0; i < runQueue.size(); ++i) {
        if (runQueue[i] != currentThread) continue;

        // Move the thread from the run queue to the wait queue
        Thread* lastThread = runQueue.back();
        runQueue[i] = lastThread;
        runQueue.pop_back();
        waitQueue.push_back({currentThread, blocker});

        ASSERT(!runQueue.empty());
        nextIdx = (i + 1) % runQueue.size();

        // We won't return from this call until we're unblocked
        yield();

        // Re-acquire the lock before returning to the previous context
        lock.lock();
        return;
    }

    panic("current thread not found in run queue");
}

void Scheduler::wakeThreads(const SharedPtr<Blocker>& blocker) {
    SpinlockLocker locker(_schedLock);

    size_t i = 0;
    while (i < waitQueue.size()) {
        BlockedThread& blockedThread = waitQueue[i];

        if (blockedThread.blocker.get() != blocker.get()) {
            ++i;
            continue;
        }

        // Move the thread from the wait queue to the run queue
        waitQueue[i] = waitQueue.back();
        waitQueue.pop_back();
        runQueue.push_back(blockedThread.thread);
    }
}
