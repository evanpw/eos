#pragma once
#include "io.h"
#include "print.h"

template <typename T, size_t N>
class RingBuffer {
public:
    RingBuffer() : _head(&_data[0]), _tail(&_data[0]) {}

    void push(const T& value) {
        // If at max capacity, drop from the head
        if (size() == N) {
            ++_head;
        }

        *_tail = value;
        _tail = increment(_tail);
    }

    T pop() {
        ASSERT(size() != 0);
        T result = *_head;
        _head = increment(_head);
        return result;
    }

    T popBack() {
        ASSERT(size() != 0);
        _tail = decrement(_tail);
        return *_tail;
    }

    const T& peek() const {
        ASSERT(size() != 0);
        return *_head;
    }

    size_t size() const {
        if (_tail >= _head) {
            return _tail - _head;
        } else {
            return (_tail + N + 1) - _head;
        }
    }

    operator bool() const { return size() != 0; }

private:
    T* increment(T* ptr) {
        ++ptr;

        if (ptr == &_data[N + 1]) {
            ptr = &_data[0];
        }

        return ptr;
    }

    T* decrement(T* ptr) {
        if (ptr == &_data[0]) {
            ptr = &_data[N];
        } else {
            --ptr;
        }

        return ptr;
    }

    T _data[N + 1];
    T* _head;
    T* _tail;
};
