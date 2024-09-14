#pragma once
#include <stdint.h>

#include "estd/vector.h"

struct Thread;

extern "C" uint64_t currentKernelStack;
extern "C" [[noreturn]] void enterContext(Thread* toThread);

struct Scheduler {
    void start();
    void run();

    void yield();
    void stopThread(Thread* thread);
    void cleanupDeadThreads();

    bool running = false;

    Vector<Thread*> runQueue;
    size_t nextIdx = 0;

    Vector<Thread*> deadQueue;
};
