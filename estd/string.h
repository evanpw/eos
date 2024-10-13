#pragma once

#include <stddef.h>

namespace estd {

class string {
public:
    string();
    string(const char* str);
    string(const char* str, size_t count);
    ~string();

    // Moveable and copyable
    string(const string& other);
    string(string&& other);
    string& operator=(const string& other);
    string& operator=(string&& other);

    bool operator==(const char* other) const;
    bool operator==(const string& other) const;

    string& operator+=(const char* other);
    string& operator+=(const string& other);

    string operator+(const char* other) const;
    string operator+(const string& other) const;

    const char* c_str() const { return _data; }
    char operator[](size_t index) const { return _data[index]; }
    size_t size() const { return _size; }

private:
    char* _data;
    size_t _size;  // not including null terminator
};

string operator+(const char* lhs, const string& rhs);

}  // namespace estd
