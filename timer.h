#pragma once
#include "trap.h"

extern "C" void irqHandler0(TrapRegisters& regs);

struct Timer {
public:
    static void init();
    static uint64_t tickCount() { return s_tickCount; }

private:
    static uint64_t s_tickCount;

    friend void irqHandler0(TrapRegisters& regs);
    static void increment() { ++s_tickCount; }
};
