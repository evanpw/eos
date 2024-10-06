#include "timer.h"

#include "interrupts.h"
#include "io.h"
#include "scheduler.h"
#include "system.h"

// 1.193182 MHz
static constexpr uint32_t PIT_FREQUENCY = 1193182;

// PIT I/O ports
enum : uint16_t {
    PIT_CHANNEL0 = 0x40,
    PIT_CHANNEL1 = 0x41,
    PIT_CHANNEL2 = 0x42,
    PIT_COMMAND = 0x43,
};

// Commands are combinations of these flags
enum : uint8_t {
    PIT_CMD_CHANNEL0 = 0,
    PIT_CMD_CHANNEL1 = 0x40,
    PIT_CMD_CHANNEL2 = 0x80,
    PIT_CMD_READBACK = 0xC0,

    PIT_CMD_LATCH = 0x00,
    PIT_CMD_LOBYTE = 0x10,
    PIT_CMD_HIBYTE = 0x20,
    PIT_CMD_BOTH = 0x30,

    PIT_CMD_MODE0 = 0x00,  // Interrupt on terminal count
    PIT_CMD_MODE1 = 0x02,  // Hardware re-triggerable one-shot
    PIT_CMD_MODE2 = 0x04,  // Rate generator
    PIT_CMD_MODE3 = 0x06,  // Square wave generator
    PIT_CMD_MODE4 = 0x08,  // Software triggered strobe
    PIT_CMD_MODE5 = 0x0A,  // Hardware triggered strobe

    PIT_CMD_BINARY = 0x00,
    PIT_CMD_BCD = 0x01,
};

Timer::Timer() {
    // Set the PIT to generate interrupts at 100Hz
    uint16_t divisor = PIT_FREQUENCY / 100;
    outb(PIT_COMMAND, PIT_CMD_CHANNEL0 | PIT_CMD_BOTH | PIT_CMD_MODE2 | PIT_CMD_BINARY);
    outb(PIT_CHANNEL0, lowBits(divisor, 8));
    outb(PIT_CHANNEL0, highBits(divisor, 8));

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
