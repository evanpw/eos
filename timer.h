#pragma once
#include "estd/atomic.h"
#include "scheduler.h"

class Timer {
public:
    Timer();

    uint64_t tickCount() { return _tickCount.load(); }
    void sleep(uint64_t duration, Spinlock* lock = nullptr);

private:
    AtomicInt _tickCount = 0;
    void increment();

    void irqHandler();

    struct TimerBlocker : Blocker {
        TimerBlocker(uint64_t endTick) : endTick(endTick) {}
        uint64_t endTick;
    };

    // TODO: should store these in sorted order, for efficiency
    estd::vector<estd::shared_ptr<TimerBlocker>> _blockers;
};
