#pragma once

#include "estd/traits.h"
#include "estd/utility.h"

namespace estd {

// Type-erased interface to any callable object
template <typename R, typename... Args>
class _function_holder_base {
public:
    virtual R call(Args... args) = 0;
};

// Hold any type of callable object and makes it callable
template <typename F, typename R, typename... Args>
class _function_holder : public _function_holder_base<R, Args...> {
    F _f;

public:
    _function_holder(F f) : _f(f) {}

    R call(Args... args) override { return _f(estd::forward<Args>(args)...); }
};

template <typename Signature>
class function;

template <typename R, typename... Args>
class function<R(Args...)> {
    _function_holder_base<R, Args...>* _holder;

public:
    function() : _holder(nullptr) {}

    template <typename F>
    function(F&& f) : _holder(new _function_holder<F, R, Args...>(f)) {}

    R operator()(Args... args) { return _holder->call(estd::forward<Args>(args)...); }

    explicit operator bool() const { return _holder != nullptr; }
};

// Deduction guides

// For function pointers
template <typename R, typename... Args>
function(R (*)(Args...)) -> function<R(Args...)>;

// For functor objects and lambdas
template <typename F>
function(F&&) -> function<get_signature_t<decltype(&remove_reference_t<F>::operator())>>;

}  // namespace estd
