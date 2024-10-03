#pragma once
#include <stddef.h>
#include <stdint.h>

#include "estd/assertions.h"
#include "estd/new.h"
#include "estd/traits.h"

// TODO: better error handling / checking

struct FormatSpec {
    int base = 10;
    size_t padTo = 0;
    char padChar = ' ';
    bool uppercase = false;
};

void printInt(const FormatSpec& spec, uint64_t value);
void printString(const FormatSpec& spec, const char* str);
void printChar(char c);

struct FormatArgBase {
    virtual ~FormatArgBase() = default;
    virtual void print(const FormatSpec& spec) const = 0;
};

template <typename T>
struct FormatArg : public FormatArgBase {};

template <>
struct FormatArg<uint64_t> : public FormatArgBase {
    FormatArg(uint64_t value) : value(value) {}

    void print(const FormatSpec& spec) const override { printInt(spec, value); }

private:
    uint64_t value;
};

template <>
struct FormatArg<const void*> : public FormatArgBase {
    FormatArg(const void* value) : value(value) {}

    void print(const FormatSpec& spec) const override { printInt(spec, (uint64_t)value); }

private:
    const void* value;
};

template <>
struct FormatArg<const char*> : public FormatArgBase {
    FormatArg(const char* value) : value(value) {}

    void print(const FormatSpec& spec) const override { printString(spec, value); }

private:
    const char* value;
};

template <typename T>
struct normalize_type {
    using type = T;
};

// Print all integral types as uint64_t
template <>
struct normalize_type<unsigned int> {
    using type = uint64_t;
};
template <>
struct normalize_type<unsigned char> {
    using type = uint64_t;
};
template <>
struct normalize_type<short unsigned int> {
    using type = uint64_t;
};
template <>
struct normalize_type<bool> {
    using type = uint64_t;
};
template <>
struct normalize_type<int> {
    using type = uint64_t;
};
template <>
struct normalize_type<long int> {
    using type = uint64_t;
};

// Specialize for char* and const char* to print as strings
template <>
struct normalize_type<const char*> {
    using type = const char*;
};
template <>
struct normalize_type<char*> {
    using type = const char*;
};

// All other pointers are converted to const void*, which is printed as an address
template <typename T>
struct normalize_type<const T*> {
    using type = const void*;
};
template <typename T>
struct normalize_type<T*> {
    using type = const void*;
};

template <typename T>
using normalize_type_t = typename normalize_type<T>::type;

class FormatArgHolder {
public:
    template <typename T>
    FormatArgHolder(T&& value) {
        using ArgType = FormatArg<normalize_type_t<estd::remove_reference_t<T>>>;
        static_assert(sizeof(ArgType) <= sizeof(_storage));
        new (_storage) ArgType(value);
    }

    operator FormatArgBase&() { return *reinterpret_cast<FormatArgBase*>(_storage); }

private:
    // Enough for one uint64_t plus a vtable ptr
    char _storage[16];
};

// To hide the template parameter of SizedFormatArgs, so that we can implement
// more of this file in the source file rather than the header
struct FormatArgs {
    virtual const FormatArgBase& next() = 0;
};

template <size_t N>
struct SizedFormatArgs : public FormatArgs {
    template <typename... Args>
    SizedFormatArgs(Args... args) : args{FormatArgHolder(args)...} {}

    FormatArgHolder args[N];
    size_t index = 0;

    const FormatArgBase& next() override {
        ASSERT(index < N);
        return args[index++];
    }
};

// Deduction guide
template <typename... Args>
SizedFormatArgs(Args...) -> SizedFormatArgs<sizeof...(Args)>;

struct FormatSpec;

struct FormatStringParser {
    FormatStringParser(const char* fmtstr) : p(fmtstr) {}

    explicit operator bool() const { return *p != '\0'; }
    char peek() const { return *p; }
    char next() { return *p++; }

    bool accept(char c);
    void expect(char c);
    size_t parseInteger();
    FormatSpec parseFormatSpec();

    const char* p;
};

void _printImpl(FormatStringParser& parser, FormatArgs& args);

template <typename... Args>
void print(const char* fmtstr, Args... args) {
    FormatStringParser parser(fmtstr);
    SizedFormatArgs formatArgs(args...);

    _printImpl(parser, formatArgs);
}

template <typename... Args>
void println(const char* fmtstr, Args... args) {
    // TODO: should use std::forward equivalent
    print(fmtstr, args...);
    printChar('\n');
}
