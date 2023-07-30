#pragma once
#include <stdint.h>

#include "estd/vector.h"

struct Thread;

extern "C" uint64_t currentKernelStack;
extern "C" [[noreturn]] void enterContext(Thread* toThread);

struct Scheduler {
    void start(Thread* initialThread);
    void run();

    Vector<Thread*> threads;
    size_t currentIdx = 0;
    bool running = false;
};
