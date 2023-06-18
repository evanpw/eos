#pragma once

template <typename T>
class Span {
public:
    Span(T* ptr, size_t size) : _data(ptr), _size(size) {}

    T* data() const { return _data; }
    size_t length() const { return _size; }

    T& operator[](size_t index) const {
        ASSERT(index < _size);
        return *(_data + index);
    }

    T* begin() const { return _data; }
    T* end() const { return _data + _size; }

private:
    T* _data;
    size_t _size;
};
