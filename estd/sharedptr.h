#pragma once
#include "estd/assertions.h"
#include "estd/stdlib.h"

template <typename T>
class SharedPtr {
    template <typename U>
    friend class SharedPtr;

public:
    SharedPtr() : _ptr(nullptr), _refCount(nullptr) {}
    explicit SharedPtr(T* ptr) : _ptr(ptr), _refCount(new int(1)) {}

    ~SharedPtr() { clear(); }

    void clear() {
        if (!_ptr) return;

        ASSERT(*_refCount > 0);
        --(*_refCount);

        if (*_refCount == 0) {
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
            _refCount = new int(1);
        }
    }

    // Copyable
    SharedPtr(const SharedPtr& other) : _ptr(other._ptr), _refCount(other._refCount) {
        if (!_ptr) return;

        ASSERT(*_refCount > 0);
        ++(*_refCount);
    }

    SharedPtr& operator=(const SharedPtr& other) {
        if (this != &other) {
            clear();

            _ptr = other._ptr;
            _refCount = other._refCount;

            if (_ptr) {
                ASSERT(*_refCount > 0);
                ++(*_refCount);
            }
        }

        return *this;
    }

    // Implicit conversion from, for example, a SharedPtr<BaseClass>
    // TODO: use constraint to make errors more legible
    template <typename U>
    SharedPtr(const SharedPtr<U>& other) : _ptr(other._ptr), _refCount(other._refCount) {
        if (!_ptr) return;

        ASSERT(*_refCount > 0);
        ++(*_refCount);
    }

    // TODO: should be moveable?

    // Access
    T* get() const { return _ptr; }
    operator bool() const { return _ptr != nullptr; }

    T* operator->() const {
        ASSERT(_ptr && *_refCount > 0);
        return _ptr;
    }

    T& operator*() const {
        ASSERT(_ptr && *_refCount > 0);
        return *_ptr;
    }

    int refCount() const {
        ASSERT(_ptr && *_refCount > 0);
        return *_refCount;
    }

private:
    T* _ptr;
    int* _refCount;
};
