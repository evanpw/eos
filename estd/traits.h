#pragma once

namespace estd {

// remove_reference maps T& and T&& to T and leaves all other types unchanged
template <typename T>
struct remove_reference {
    using type = T;
};

template <typename T>
struct remove_reference<T&> {
    using type = T;
};

template <typename T>
struct remove_reference<T&&> {
    using type = T;
};

template <typename T>
using remove_reference_t = typename remove_reference<T>::type;

// get_signature maps a function pointer type to its signature (function type)
template <typename T>
struct get_signature {};

template <typename R, typename... Args>
struct get_signature<R (*)(Args...)> {
    using type = R(Args...);
};

template <typename C, typename R, typename... Args>
struct get_signature<R (C::*)(Args...)> {
    using type = R(Args...);
};

template <typename C, typename R, typename... Args>
struct get_signature<R (C::*)(Args...) const> {
    using type = R(Args...);
};

template <typename T>
using get_signature_t = typename get_signature<T>::type;

// enable_if is a map from bool to types, which maps true to void and produces a
// substitution failure for false
template <bool B>
struct enable_if {};

template <>
struct enable_if<true> {
    using type = void;
};

template <bool B>
using enable_if_t = typename enable_if<B>::type;

// is_integral is a function from types to bool, which is true for integer types and false
// otherwise
template <typename T>
inline constexpr bool is_integral_v = false;
template <>
inline constexpr bool is_integral_v<bool> = true;
template <>
inline constexpr bool is_integral_v<char> = true;
template <>
inline constexpr bool is_integral_v<signed char> = true;
template <>
inline constexpr bool is_integral_v<unsigned char> = true;
template <>
inline constexpr bool is_integral_v<wchar_t> = true;
template <>
inline constexpr bool is_integral_v<char8_t> = true;
template <>
inline constexpr bool is_integral_v<char16_t> = true;
template <>
inline constexpr bool is_integral_v<char32_t> = true;
template <>
inline constexpr bool is_integral_v<short> = true;
template <>
inline constexpr bool is_integral_v<unsigned short> = true;
template <>
inline constexpr bool is_integral_v<int> = true;
template <>
inline constexpr bool is_integral_v<unsigned int> = true;
template <>
inline constexpr bool is_integral_v<long> = true;
template <>
inline constexpr bool is_integral_v<unsigned long> = true;
template <>
inline constexpr bool is_integral_v<long long> = true;
template <>
inline constexpr bool is_integral_v<unsigned long long> = true;
template <typename T>
inline constexpr bool is_integral_v<const T> = is_integral_v<T>;

// remove_const is a function from types to types which maps const T to T and leaves all
// other types alone
template <typename T>
struct remove_const {
    using type = T;
};

template <typename T>
struct remove_const<const T> {
    using type = T;
};

template <typename T>
using remove_const_t = typename remove_const<T>::type;

// add_const is a function from types to types which sends T to const T
template <typename T>
struct add_const {
    using type = const T;
};

template <typename T>
using add_const_t = typename add_const<T>::type;

// Generic function which returns a copy of its argument (useful for passing packed fields
// to a function which takes a forwarding reference, like estd::print)
template <typename T>
constexpr T make_copy(const T& t) noexcept {
    return t;
}

}  // namespace estd
