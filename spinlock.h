// Defines a spinlock and utility classes
#pragma once

#include "estd/assertions.h"
#include "estd/atomic.h"
#include "processor.h"

class Spinlock {
public:
    Spinlock() = default;

    // No copy / move
    Spinlock(const Spinlock&) = delete;
    Spinlock& operator=(const Spinlock&) = delete;
    Spinlock(Spinlock&&) = delete;
    Spinlock& operator=(Spinlock&&) = delete;

    void lock() {
        _flag = Processor::saveAndDisableInterrupts();

        while (_locked.exchange(true)) {
            // Gives hint to the processor that this is a spin-wait loop
            Processor::pause();
        }
    }

    void unlock(bool restoreInterrupts = true) {
        ASSERT(isLocked());
        _locked.store(false);
        if (restoreInterrupts) Processor::restoreInterrupts(_flag);
    }

    // Like lock(), but doesn't overwrite the previous interrupts state
    void relock() {
        Processor::disableInterrupts();

        while (_locked.exchange(true)) {
            // Gives hint to the processor that this is a spin-wait loop
            Processor::pause();
        }
    }

    bool isLocked() { return _locked.load(); }

private:
    AtomicBool _locked;
    InterruptsFlag _flag;
};

// Locks a spinlock when created, unlocks when destroyed
class SpinlockLocker {
public:
    SpinlockLocker(Spinlock& lock) : _lock(lock) { _lock.lock(); }
    ~SpinlockLocker() { _lock.unlock(); }

    // No copy / move
    SpinlockLocker(const SpinlockLocker&) = delete;
    SpinlockLocker& operator=(const SpinlockLocker&) = delete;

private:
    Spinlock& _lock;
};

// Unlocks a spinlock when created, relocks when destroyed
class SpinlockUnlocker {
public:
    SpinlockUnlocker(Spinlock& lock) : _lock(lock) { _lock.unlock(false); }

    ~SpinlockUnlocker() { _lock.relock(); }

    // No copy / move
    SpinlockUnlocker(const SpinlockUnlocker&) = delete;
    SpinlockUnlocker& operator=(const SpinlockUnlocker&) = delete;

private:
    Spinlock& _lock;
};
