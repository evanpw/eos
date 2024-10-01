#include "timer.h"

#include "interrupts.h"
#include "io.h"
#include "scheduler.h"
#include "system.h"

Timer::Timer() {
    registerIrqHandler(0, [this](TrapRegisters&) { this->irqHandler(); });
}

void Timer::sleep(uint64_t duration, Spinlock* lock) {
    uint64_t endTick = tickCount() + duration;

    estd::shared_ptr<TimerBlocker> blocker(new TimerBlocker{endTick});
    _blockers.push_back(blocker);

    sys.scheduler().sleepThread(blocker, lock);
}

void Timer::increment() {
    uint64_t newTickCount = _tickCount.increment();

    size_t i = 0;
    while (i < _blockers.size()) {
        auto& blocker = _blockers[i];

        if (newTickCount < blocker->endTick) {
            ++i;
            continue;
        }

        sys.scheduler().wakeThreads(blocker);

        // Remove the expired blocker from the list
        _blockers[i] = _blockers.back();
        _blockers.pop_back();
    }
}

void Timer::irqHandler() {
    increment();
    outb(PIC1_COMMAND, EOI);
    sys.scheduler().onTimerInterrupt();
}
