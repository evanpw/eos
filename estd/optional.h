#pragma once

#include "estd/utility.h"

namespace estd {

template <typename T>
class optional {
public:
    optional() = default;
    optional(const T& value) : _has_value(true) { new (_data) T(value); }
    optional(T&& value) : _has_value(true) { new (_data) T(estd::move(value)); }

    optional(const optional& other) : _has_value(other._has_value) {
        if (_has_value) {
            new (_data) T(*other);
        }
    }

    optional(optional&& other) : _has_value(other._has_value) {
        if (_has_value) {
            new (_data) T(estd::move(*other));
        }
    }

    optional& operator=(const optional& other) {
        if (this != &other) {
            set_value(other.value());
        }

        return *this;
    }

    optional& operator=(optional&& other) {
        if (this != &other) {
            set_value(estd::move(other).value());
        }

        return *this;
    }

    optional& operator=(const T& value) {
        set_value(value);
        return *this;
    }

    optional& operator=(T&& value) {
        set_value(estd::move(value));
        return *this;
    }

    T& value() & { return *reinterpret_cast<T*>(_data); }
    const T& value() const& { return *reinterpret_cast<const T*>(_data); }
    T&& value() && { return estd::move(*reinterpret_cast<T*>(_data)); }
    const T&& value() const&& { return estd::move(*reinterpret_cast<const T*>(_data)); }

    const T* operator->() const { return reinterpret_cast<const T*>(_data); }
    T* operator->() { return reinterpret_cast<T*>(_data); }
    const T& operator*() const& { return value(); }
    T& operator*() & { return value(); }
    const T&& operator*() const&& { return estd::move(value()); }
    T&& operator*() && { return estd::move(value()); }

    explicit operator bool() { return _has_value; }

private:
    void set_value(const T& value) {
        if (_has_value) {
            this->value() = value;
        } else {
            new (_data) T(value);
            _has_value = true;
        }
    }

    void set_value(T&& value) {
        if (_has_value) {
            this->value() = estd::move(value);
        } else {
            new (_data) T(estd::move(value));
            _has_value = true;
        }
    }

    char _data[sizeof(T)] = {};
    bool _has_value = false;
};

}  // namespace estd
