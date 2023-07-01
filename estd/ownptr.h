#pragma once
#include "estd/stdlib.h"

// This template is used so that we call "delete arg" on a regular pointer,
// but "delete[] arg" on a pointer to an array
template <typename T>
struct Deleter {
    void operator()(T* arg) {
       delete arg;
    }
};

template <typename T>
struct Deleter<T[]> {
    void operator()(T* arg) {
        delete[] arg;
    }
};

template <typename T>
class OwnPtr {
public:
    OwnPtr() = default;
    explicit OwnPtr(T* ptr) : _ptr(ptr) {}

    ~OwnPtr() {
        Deleter<T>{}(_ptr);
        _ptr = nullptr;
    }

    [[nodiscard]] T* release() {
        T* ptr = _ptr;
        _ptr = nullptr;
        return ptr;
    }

    // Non-copyable
    OwnPtr(const OwnPtr&) = delete;
    OwnPtr& operator=(const OwnPtr&) = delete;

    // Movable
    OwnPtr(OwnPtr&& other) : _ptr(other.release()) {
    }

    OwnPtr& operator=(OwnPtr&& other) {
        _ptr = other.release();
        return *this;
    }

    // Access
    T* get() const { return _ptr; }
    operator T*() const { return _ptr; }
    operator bool() const { return _ptr != nullptr; }
    T* operator->() const { return _ptr; }
    T& operator*() const { return *_ptr; }

private:
    T* _ptr = nullptr;
};
