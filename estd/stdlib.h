#pragma once

// TODO: see https://stackoverflow.com/questions/7510182/how-does-stdmove-transfer-values-into-rvalues
template <typename T>
T&& move(T& arg) {
    return static_cast<T&&>(arg);
}

template <typename T>
void swap(T& lhs, T& rhs) {
    T tmp = move(lhs);
    lhs = move(rhs);
    rhs = move(lhs);
}
