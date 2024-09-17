#pragma once
#include "estd/assertions.h"
#include "estd/new.h"
#include "estd/stdlib.h"

template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;

    Vector() : _data(nullptr), _capacity(0), _size(0) {}

    Vector(size_t count, const T& value = T()) {
        _capacity = count;
        _size = count;
        _data = static_cast<T*>(::operator new[](sizeof(T) * _capacity));

        for (size_t i = 0; i < _size; ++i) {
            new (&_data[i]) T(value);
        }
    }

    ~Vector() {
        for (size_t i = 0; i < _size; ++i) {
            _data[i].~T();
        }

        ::operator delete[](_data);
    }

    size_t size() const { return _size; }
    size_t capacity() const { return _capacity; }
    T* data() { return _data; }
    const T* data() const { return _data; }
    bool empty() const { return _size == 0; }

    void reserve(size_t newCapacity) {
        if (newCapacity < _capacity) return;

        T* newData = static_cast<T*>(::operator new[](sizeof(T) * newCapacity));

        for (size_t i = 0; i < _size; ++i) {
            new (&newData[i]) T(move(_data[i]));
            _data[i].~T();
        }

        ::operator delete[](_data);
        _data = newData;
        _capacity = newCapacity;
    }

    void push_back(const T& value) {
        if (_size == _capacity) {
            reserve(_capacity == 0 ? 1 : 2 * _capacity);
        }

        ASSERT(_capacity > _size);
        new (&_data[_size++]) T(value);
    }

    void push_back(T&& value) {
        if (_size == _capacity) {
            reserve(_capacity == 0 ? 1 : 2 * _capacity);
        }

        ASSERT(_capacity > _size);
        new (&_data[_size++]) T(move(value));
    }

    void pop_back() {
        ASSERT(_size > 0);
        _data[_size - 1].~T();
        --_size;
    }

    void clear() {
        for (size_t i = 0; i < _size; ++i) {
            _data[i].~T();
        }

        _size = 0;
    }

    const T& operator[](size_t index) const {
        ASSERT(index < _size);
        return _data[index];
    }

    T& operator[](size_t index) {
        ASSERT(index < _size);
        return _data[index];
    }

    T& back() {
        ASSERT(_size > 0);
        return _data[_size - 1];
    }

    iterator begin() { return data(); }
    iterator end() { return data() + size(); }
    const_iterator cbegin() { return data(); }
    const_iterator cend() { return data() + size(); }

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
