#pragma once
#include "estd/atomic.h"

namespace estd {

template <typename T>
class shared_ptr {
    template <typename U>
    friend class shared_ptr;

public:
    shared_ptr() : _ptr(nullptr), _refCount(nullptr) {}
    explicit shared_ptr(T* ptr) : _ptr(ptr), _refCount(new AtomicInt(1)) {}

    ~shared_ptr() { clear(); }

    void clear() {
        if (!_ptr) return;

        if (_refCount->decrement() == 0) {
            delete _ptr;
            delete _refCount;
        }

        _ptr = nullptr;
        _refCount = nullptr;
    }

    void assign(T* ptr) {
        clear();

        if (ptr) {
            _ptr = ptr;
            _refCount = new AtomicInt(1);
        }
    }

    // Copyable
    shared_ptr(const shared_ptr& other) : _ptr(other._ptr), _refCount(other._refCount) {
        if (!_ptr) return;
        _refCount->increment();
    }

    shared_ptr& operator=(const shared_ptr& other) {
        if (_ptr != other._ptr) {
            clear();

            _ptr = other._ptr;
            _refCount = other._refCount;

            if (_ptr) {
                _refCount->increment();
            }
        }

        return *this;
    }

    // Implicit conversion from, for example, a shared_ptr<BaseClass>
    // TODO: use constraint to make errors more legible
    template <typename U>
    shared_ptr(const shared_ptr<U>& other)
    : _ptr(other._ptr), _refCount(other._refCount) {
        if (!_ptr) return;
        _refCount->increment();
    }

    // TODO: should be moveable?

    // Access
    T* get() const { return _ptr; }
    operator bool() const { return _ptr != nullptr; }
    T* operator->() const { return _ptr; }
    T& operator*() const { return *_ptr; }
    int refCount() const { return _refCount->load(); }

private:
    T* _ptr;
    AtomicInt* _refCount;
};

}  // namespace estd
