// Implementation of a small subset of libc and related function for use in
// the kernel
#pragma once

template <typename T>
T min(const T& lhs, const T& rhs) {
    return (rhs < lhs) ? rhs : lhs;
}

template <typename T>
T max(const T& lhs, const T& rhs) {
    return (lhs < rhs) ? rhs : lhs;
}
