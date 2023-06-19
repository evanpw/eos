#pragma once
#include "estd/assertions.h"

template <typename T>
class Vector {
public:
    Vector() : _data(nullptr), _capacity(0), _size(0) {}

    Vector(size_t count, const T& value = T()) {
        _capacity = count;
        _size = count;
        _data = new T[_capacity];

        for (size_t i = 0; i < _size; ++i) {
            _data[i] = value;
        }
    }

    ~Vector() { delete _data; }

    size_t size() const { return _size; }
    size_t capacity() const { return _capacity; }
    T* data() { return _data; }
    const T* data() const { return _data; }

    void reserve(size_t newCapacity) {
        if (newCapacity < _capacity) return;

        T* newData = new T[newCapacity];

        for (size_t i = 0; i < _size; ++i) {
            newData[i] = _data[i];
        }

        _data = newData;
        _capacity = newCapacity;
    }

    void push_back(const T& value) {
        if (_size == _capacity) {
            reserve(_capacity == 0 ? 1 : 2 * _capacity);
        }

        ASSERT(_capacity > _size);
        _data[_size++] = value;
    }

    const T& operator[](size_t index) const {
        ASSERT(index < _capacity);
        return _data[index];
    }

    T& operator[](size_t index) {
        ASSERT(index < _capacity);
        return _data[index];
    }

    // TODO: implement these
    Vector(const Vector&) = delete;
    Vector(Vector&&) = delete;
    Vector& operator=(const Vector&) = delete;
    Vector& operator=(Vector&&) = delete;

private:
    T* _data;
    size_t _capacity;
    size_t _size;
};
