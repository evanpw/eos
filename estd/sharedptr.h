#pragma once
#include "estd/atomic.h"

template <typename T>
class SharedPtr {
    template <typename U>
    friend class SharedPtr;

public:
    SharedPtr() : _ptr(nullptr), _refCount(nullptr) {}
    explicit SharedPtr(T* ptr) : _ptr(ptr), _refCount(new AtomicInt(1)) {}

    ~SharedPtr() { clear(); }

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
    SharedPtr(const SharedPtr& other) : _ptr(other._ptr), _refCount(other._refCount) {
        if (!_ptr) return;
        _refCount->increment();
    }

    SharedPtr& operator=(const SharedPtr& other) {
        if (this != &other) {
            clear();

            _ptr = other._ptr;
            _refCount = other._refCount;

            if (_ptr) {
                _refCount->increment();
            }
        }

        return *this;
    }

    // Implicit conversion from, for example, a SharedPtr<BaseClass>
    // TODO: use constraint to make errors more legible
    template <typename U>
    SharedPtr(const SharedPtr<U>& other) : _ptr(other._ptr), _refCount(other._refCount) {
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
