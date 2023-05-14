#pragma once
#include "assertions.h"
#include <stdint.h>

// TODO: better error handling / checking

// To hide the template paramter of SizedFormatArgs, so that we can implement
// more of this file in the source file rather than the header
struct FormatArgs {
    virtual uint64_t next() = 0;
};

template <size_t N>
struct SizedFormatArgs : public FormatArgs {
    template <typename... Args>
    SizedFormatArgs(Args... args) : args{args...} {}

    uint64_t args[N];
    size_t index = 0;

    uint64_t next() override {
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
    SizedFormatArgs formatArgs(static_cast<uint64_t>(args)...);

    _printImpl(parser, formatArgs);
}

template <typename... Args>
void println(const char* fmtstr, Args... args) {
    // TODO: should use std::forward here
    print(fmtstr, args...);
    printChar('\n');
}
