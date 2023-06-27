#pragma once
#include "interrupts.h"

void __attribute__((interrupt)) irqHandler0(InterruptFrame* frame);

struct Timer {
public:
    static void init() { s_tickCount = 0; }
    static uint64_t tickCount() { return s_tickCount; }

private:
    static uint64_t s_tickCount;

    friend void __attribute__((interrupt)) irqHandler0(InterruptFrame* frame);
    static void increment() { ++s_tickCount; }
};
