#pragma once
#include <stddef.h>
#include <stdint.h>

#include "estd/assertions.h"
#include "estd/new.h"
#include "estd/stddef.h"
#include "estd/traits.h"
#include "estd/utility.h"

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

template <typename T, typename = void>
struct FormatArg : public FormatArgBase {};

template <typename T>
struct FormatArg<T, estd::enable_if_t<estd::is_integral_v<T>>> : public FormatArgBase {
    FormatArg(T value) : value(value) {}

    void print(const FormatSpec& spec) const override { printInt(spec, (uint64_t)value); }

private:
    T value;
};

template <>
struct FormatArg<void*> : public FormatArgBase {
    FormatArg(const void* value) : value(value) {}

    void print(const FormatSpec& spec) const override { printInt(spec, (uint64_t)value); }

private:
    const void* value;
};

template <>
struct FormatArg<char*> : public FormatArgBase {
    FormatArg(const char* value) : value(value) {}

    void print(const FormatSpec& spec) const override { printString(spec, value); }

private:
    const char* value;
};

template <>
struct FormatArg<byte> : public FormatArgBase {
    FormatArg(byte value) : value(value) {}

    void print(const FormatSpec&) const override {
        FormatSpec spec = {.base = 16, .padTo = 2, .padChar = '0', .uppercase = true};
        printInt(spec, (uint64_t)value);
    }

private:
    byte value;
};

template <typename T>
struct normalize_type {
    using type = T;
};
template <typename T>
struct normalize_type<const T> {
    using type = typename normalize_type<T>::type;
};
template <typename T>
struct normalize_type<T&> {
    using type = typename normalize_type<T>::type;
};
template <typename T>
struct normalize_type<T&&> {
    using type = typename normalize_type<T>::type;
};
template <typename T>
struct normalize_type<const T*> {
    using type = typename normalize_type<T*>::type;
};

// Print char pointers or arrays as strings
template <>
struct normalize_type<char*> {
    using type = char*;
};
template <size_t N>
struct normalize_type<char[N]> {
    using type = char*;
};
template <>
struct normalize_type<char[]> {
    using type = char*;
};

// All other pointers are converted to const void* and printed as an address
template <typename T>
struct normalize_type<T*> {
    using type = void*;
};

template <typename T>
using normalize_type_t = typename normalize_type<T>::type;

class FormatArgHolder {
public:
    template <typename T>
    FormatArgHolder(T&& value) {
        using ArgType = FormatArg<normalize_type_t<T>>;
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
    SizedFormatArgs(Args&&... args)
    : args{FormatArgHolder(estd::forward<Args>(args))...} {}

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
void print(const char* fmtstr, Args&&... args) {
    FormatStringParser parser(fmtstr);
    SizedFormatArgs formatArgs(estd::forward<Args>(args)...);

    _printImpl(parser, formatArgs);
}

template <typename... Args>
void println(const char* fmtstr, Args&&... args) {
    print(fmtstr, estd::forward<Args>(args)...);
    printChar('\n');
}
