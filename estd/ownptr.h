#pragma once
#include "estd/assertions.h"
#include "estd/stdlib.h"

template <typename T>
class OwnPtr {
public:
    OwnPtr() = default;
    explicit OwnPtr(T* ptr) : _ptr(ptr) {}
    ~OwnPtr() { clear(); }

    void clear() {
        delete _ptr;
        _ptr = nullptr;
    }

    void assign(T* ptr) {
        clear();
        _ptr = ptr;
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
    OwnPtr(OwnPtr&& other) : _ptr(other.release()) {}

    OwnPtr& operator=(OwnPtr&& other) {
        _ptr = other.release();
        return *this;
    }

    // Access
    T* get() const { return _ptr; }
    operator bool() const { return _ptr != nullptr; }

    T* operator->() const {
        ASSERT(_ptr);
        return _ptr;
    }

    T& operator*() const {
        ASSERT(_ptr);
        return *_ptr;
    }

private:
    T* _ptr = nullptr;
};

template <typename T>
class OwnPtr<T[]> {
public:
    OwnPtr() = default;
    explicit OwnPtr(T* ptr) : _ptr(ptr) {}
    ~OwnPtr() { clear(); }

    void clear() {
        delete[] _ptr;
        _ptr = nullptr;
    }

    void assign(T* ptr) {
        clear();
        _ptr = ptr;
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
    OwnPtr(OwnPtr&& other) : _ptr(other.release()) {}

    OwnPtr& operator=(OwnPtr&& other) {
        _ptr = other.release();
        return *this;
    }

    // Access
    T* get() const { return _ptr; }
    operator bool() const { return _ptr != nullptr; }

    T& operator[](size_t index) const {
        ASSERT(_ptr);
        return _ptr[index];
    }

private:
    T* _ptr = nullptr;
};
