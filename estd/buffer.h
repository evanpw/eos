#pragma once
#include <stdint.h>
#include <string.h>

#include "estd/assertions.h"
#include "estd/utility.h"

// Fixed (runtime) length, heap-allocated array of bytes
class Buffer {
public:
    Buffer() = default;

    Buffer(size_t count) {
        _data = new uint8_t[count];
        _size = count;
        memset(_data, 0, count);
    }

    ~Buffer() {
        delete[] _data;
        _data = nullptr;
        _size = 0;
    }

    // Non-copyable
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    // Moveable
    Buffer(Buffer&& other) { swap(other); }

    Buffer& operator=(Buffer&& other) {
        swap(other);
        return *this;
    }

    operator bool() const { return _data != nullptr; }
    size_t size() const { return _size; }
    uint8_t* get() { return _data; }
    const uint8_t* get() const { return _data; }

    const uint8_t& operator[](size_t index) const {
        ASSERT(index < _size);
        return _data[index];
    }

    uint8_t& operator[](size_t index) {
        ASSERT(index < _size);
        return _data[index];
    }

private:
    void swap(Buffer& other) {
        estd::swap(_data, other._data);
        estd::swap(_size, other._size);
    }

    uint8_t* _data = 0;
    size_t _size = 0;
};
