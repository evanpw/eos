// Defines a spinlock and utility classes
#pragma once

#include "estd/assertions.h"
#include "estd/atomic.h"
#include "interrupts.h"

class Spinlock {
public:
    Spinlock() = default;

    // No copy / move
    Spinlock(const Spinlock&) = delete;
    Spinlock& operator=(const Spinlock&) = delete;
    Spinlock(Spinlock&&) = delete;
    Spinlock& operator=(Spinlock&&) = delete;

    InterruptsFlag lock() {
        InterruptsFlag flag = Interrupts::saveAndDisable();

        // This isn't really necessary on a single processor. Disabling
        // interrupts is enough already
        while (_locked.exchange(true)) {
            // Gives hint to the processor that this is a spin-wait loop
            asm volatile("pause");
        }

        return flag;
    }

    void unlock(InterruptsFlag flag) {
        ASSERT(isLocked());
        _locked.store(false);
        Interrupts::restore(flag);
    }

    bool isLocked() { return _locked.load(); }

private:
    AtomicBool _locked;
};

// Locks a spinlock when created, unlocks when destroyed
class SpinlockLocker {
public:
    SpinlockLocker(Spinlock& lock) : _lock(lock) {
        _flag = _lock.lock();
        _haveLock = true;
    }

    ~SpinlockLocker() {
        if (_haveLock) {
            _lock.unlock(_flag);
        }
    }

    void unlock() {
        ASSERT(_haveLock);
        _haveLock = false;
        _lock.unlock(_flag);
    }

    // No copy / move
    SpinlockLocker(const SpinlockLocker&) = delete;
    SpinlockLocker& operator=(const SpinlockLocker&) = delete;
    SpinlockLocker(SpinlockLocker&&) = delete;
    SpinlockLocker& operator=(SpinlockLocker&&) = delete;

private:
    Spinlock& _lock;
    InterruptsFlag _flag;
    bool _haveLock = false;
};
