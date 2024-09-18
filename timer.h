#pragma once
#include "estd/atomic.h"
#include "scheduler.h"
#include "trap.h"

extern "C" void irqHandler0(TrapRegisters& regs);

class Timer {
    friend void irqHandler0(TrapRegisters& regs);

public:
    Timer();

    uint64_t tickCount() { return _tickCount.load(); }
    void sleep(uint64_t duration);

private:
    AtomicInt _tickCount = 0;
    void increment();

    struct TimerBlocker : Blocker {
        TimerBlocker(uint64_t endTick) : endTick(endTick) {}
        uint64_t endTick;
    };

    // TODO: should store these in sorted order, for efficiency
    Vector<SharedPtr<TimerBlocker>> _blockers;
};
