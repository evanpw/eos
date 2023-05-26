#pragma once
#include <stdint.h>

#include "assertions.h"

// TODO: better error handling / checking

struct FormatSpec;

struct FormatArg {
    FormatArg(uint64_t value) : value(value) {}
    FormatArg(void* value) : value(reinterpret_cast<uint64_t>(value)) {}

    void print(const FormatSpec& spec);

    uint64_t value;
};

// To hide the template paramter of SizedFormatArgs, so that we can implement
// more of this file in the source file rather than the header
struct FormatArgs {
    virtual FormatArg next() = 0;
};

template <size_t N>
struct SizedFormatArgs : public FormatArgs {
    template <typename... Args>
    SizedFormatArgs(Args... args) : args{FormatArg(args)...} {}

    FormatArg args[N];
    size_t index = 0;

    FormatArg next() override {
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

void printChar(char c);
void _printImpl(FormatStringParser& parser, FormatArgs& args);

template <typename... Args>
void print(const char* fmtstr, Args... args) {
    FormatStringParser parser(fmtstr);
    SizedFormatArgs formatArgs(args...);

    _printImpl(parser, formatArgs);
}

template <typename... Args>
void println(const char* fmtstr, Args... args) {
    // TODO: should use std::forward here
    print(fmtstr, args...);
    printChar('\n');
}
