#pragma once

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
