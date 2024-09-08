// Implements a small subset of libc and related functions for use in the
// kernel
#pragma once

template <typename T>
T min(const T& lhs, const T& rhs) {
    return (rhs < lhs) ? rhs : lhs;
}

template <typename T>
T max(const T& lhs, const T& rhs) {
    return (lhs < rhs) ? rhs : lhs;
}

template <typename T, typename U>
T bit_cast(const U& u) {
    return __builtin_bit_cast(T, u);
}

template <typename T, typename U>
T ceilDiv(T num, U denom) {
    ASSERT(num >= 0 && denom > 0);
    return (num + denom - 1) / denom;
}

template <typename T, typename U>
T roundUp(T num, U denom) {
    ASSERT(num >= 0 && denom > 0);
    return denom * ceilDiv(num, denom);
}

template <typename T, typename U>
T roundDown(T num, U denom) {
    ASSERT(num >= 0 && denom > 0);
    return denom * (num / denom);
}
