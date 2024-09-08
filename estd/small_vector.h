#pragma once

#include <stddef.h>

#include "estd/assertions.h"

// Like a std::vector, but with a small, inline, compile-time capacity
template <typename T, size_t N>
class SmallVector {
public:
    SmallVector() = default;

    // Not copyable
    SmallVector(const SmallVector&) = delete;
    SmallVector& operator=(const SmallVector&) = delete;

    void pushBack(const T& value) {
        ASSERT(_size + 1 < N);
        _data[_size++] = value;
    }

    void clear() { _size = 0; }

    size_t size() const { return _size; }
    bool empty() const { return _size == 0; }
    bool full() const { return _size + 1 == N; }

    T& operator[](size_t index) {
        ASSERT(index < _size);
        return _data[index];
    }

private:
    T _data[N];
    size_t _size = 0;
};
