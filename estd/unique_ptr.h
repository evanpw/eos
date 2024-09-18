#pragma once
#include "estd/assertions.h"

namespace estd {

template <typename T>
class unique_ptr {
public:
    unique_ptr() = default;
    explicit unique_ptr(T* ptr) : _ptr(ptr) {}
    ~unique_ptr() { clear(); }

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
    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;

    // Movable
    unique_ptr(unique_ptr&& other) : _ptr(other.release()) {}

    unique_ptr& operator=(unique_ptr&& other) {
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
class unique_ptr<T[]> {
public:
    unique_ptr() = default;
    explicit unique_ptr(T* ptr) : _ptr(ptr) {}
    ~unique_ptr() { clear(); }

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
    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;

    // Movable
    unique_ptr(unique_ptr&& other) : _ptr(other.release()) {}

    unique_ptr& operator=(unique_ptr&& other) {
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

}  // namespace estd
