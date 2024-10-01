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

}  // namespace estd
