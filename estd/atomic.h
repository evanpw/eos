#pragma once

#include <stdint.h>

class AtomicBool {
public:
    AtomicBool() noexcept = default;

    // No copy or move
    AtomicBool& operator=(const AtomicBool&) volatile = delete;
    AtomicBool& operator=(AtomicBool&&) volatile = delete;
    AtomicBool(const AtomicBool&) = delete;
    AtomicBool(AtomicBool&&) = delete;

    bool load() const volatile noexcept {
        return __atomic_load_n(&_value, __ATOMIC_ACQUIRE);
    }

    void store(bool value) volatile noexcept {
        __atomic_store_n(&_value, value, __ATOMIC_RELEASE);
    }

    bool exchange(bool desired) volatile noexcept {
        return __atomic_exchange_n(&_value, desired, __ATOMIC_ACQUIRE);
    }

private:
    bool _value = false;
};

static_assert(sizeof(AtomicBool) == sizeof(bool));

class AtomicInt {
public:
    AtomicInt(uint64_t value = 0) noexcept : _value(value) {}

    // No copy or move
    AtomicInt& operator=(const AtomicInt&) volatile = delete;
    AtomicInt& operator=(AtomicInt&&) volatile = delete;
    AtomicInt(const AtomicInt&) = delete;
    AtomicInt(AtomicInt&&) = delete;

    uint64_t load() const volatile noexcept {
        return __atomic_load_n(&_value, __ATOMIC_ACQUIRE);
    }

    void store(uint64_t value) volatile noexcept {
        __atomic_store_n(&_value, value, __ATOMIC_RELEASE);
    }

    uint64_t increment() volatile noexcept {
        return __atomic_add_fetch(&_value, 1, __ATOMIC_ACQ_REL);
    }

    uint64_t decrement() volatile noexcept {
        return __atomic_sub_fetch(&_value, 1, __ATOMIC_ACQ_REL);
    }

private:
    uint64_t _value = 0;
};

static_assert(sizeof(AtomicInt) == sizeof(uint64_t));
