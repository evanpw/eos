#pragma once

namespace estd {

// https://stackoverflow.com/questions/7510182/how-does-stdmove-transfer-values-into-rvalues
template <typename T>
T&& move(T& arg) noexcept {
    return static_cast<T&&>(arg);
}

template <typename T>
T&& move(T&& arg) noexcept {
    return static_cast<T&&>(arg);
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

}  // namespace estd
