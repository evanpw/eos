// Defines a spinlock and utility classes
#pragma once

#include "estd/assertions.h"
#include "estd/shared_ptr.h"
#include "scheduler.h"
#include "system.h"

class Mutex {
public:
    Mutex() : _blocker(new Blocker) {}

    // No copy / move
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

    void lock() {
        SpinlockLocker locker(_spinlock);
        while (_locked) {
            sys.scheduler().sleepThread(_blocker, &_spinlock);
        }
        _locked = true;
    }

    void unlock() {
        SpinlockLocker locker(_spinlock);
        ASSERT(_locked);
        _locked = false;
        sys.scheduler().wakeThreads(_blocker);
    }

    bool isLocked() {
        SpinlockLocker locker(_spinlock);
        return _locked;
    }

private:
    Spinlock _spinlock;
    bool _locked;
    estd::shared_ptr<Blocker> _blocker;
};

// Locks a mutex when created, unlocks when destroyed
class MutexLocker {
public:
    MutexLocker(Mutex& lock) : _lock(lock) { _lock.lock(); }
    ~MutexLocker() { _lock.unlock(); }

    // No copy / move
    MutexLocker(const MutexLocker&) = delete;
    MutexLocker& operator=(const MutexLocker&) = delete;

private:
    Mutex& _lock;
};
