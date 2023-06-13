#pragma once

#include "assertions.h"
#include "atomic.h"
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
