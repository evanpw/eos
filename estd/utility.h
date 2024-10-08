#pragma once

#include "estd/traits.h"

namespace estd {

// https://stackoverflow.com/questions/7510182/how-does-stdmove-transfer-values-into-rvalues
template <typename T>
constexpr remove_reference_t<T>&& move(T&& arg) noexcept {
    return static_cast<remove_reference_t<T>&&>(arg);
}

template <typename T>
void swap(T& lhs, T& rhs) {
    T tmp = move(lhs);
    lhs = move(rhs);
    rhs = move(tmp);
}

template <typename T1, typename T2>
struct pair {
    T1 first;
    T2 second;

    pair() = default;
    pair(const T1& first, const T2& second) : first(first), second(second) {}
    pair(T1&& first, T2&& second) : first(move(first)), second(move(second)) {}
    pair(const pair& other) : first(other.first), second(other.second) {}
    pair(pair&& other) : first(move(other.first)), second(move(other.second)) {}

    pair& operator=(const pair& other) {
        if (&other != this) {
            first = other.first;
            second = other.second;
        }

        return *this;
    }

    pair& operator=(pair&& other) {
        if (&other != this) {
            first = move(other.first);
            second = move(other.second);
        }

        return *this;
    }
};

template <typename T1, typename T2>
pair<T1, T2> make_pair(T1&& first, T2&& second) {
    return pair<T1, T2>(move(first), move(second));
}

template <class T>
constexpr T&& forward(remove_reference_t<T>& t) noexcept {
    return static_cast<T&&>(t);
}

template <class T>
constexpr T&& forward(remove_reference_t<T>&& t) noexcept {
    return static_cast<T&&>(t);
}

}  // namespace estd
