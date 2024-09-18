#include "timer.h"

#include "interrupts.h"
#include "io.h"
#include "scheduler.h"
#include "system.h"

void irqHandler0(TrapRegisters&) {
    System::timer().increment();
    outb(PIC1_COMMAND, EOI);
    System::scheduler().onTimerInterrupt();
}

Timer::Timer() { registerIrqHandler(0, irqHandler0); }

void Timer::sleep(uint64_t duration) {
    uint64_t endTick = tickCount() + duration;

    SharedPtr<TimerBlocker> blocker(new TimerBlocker{endTick});
    _blockers.push_back(blocker);

    System::scheduler().sleepThread(blocker);
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

        System::scheduler().wakeThreads(blocker);

        // Remove the expired blocker from the list
        _blockers[i] = _blockers.back();
        _blockers.pop_back();
    }
}
