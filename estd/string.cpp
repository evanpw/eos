#include "estd/string.h"

#include <string.h>

namespace estd {

string::string() {
    _size = 0;
    _data = new char[1];
    _data[0] = '\0';
}

string::string(const char* str) {
    _size = strlen(str);
    _data = new char[_size + 1];
    strcpy(_data, str);
}

string::string(const char* str, size_t count) {
    _size = count;
    _data = new char[_size + 1];
    memcpy(_data, str, count);
    _data[count] = '\0';
}

string::~string() { delete[] _data; }

string::string(const string& other) {
    _size = other._size;
    _data = new char[_size + 1];
    strcpy(_data, other._data);
}

string::string(string&& other) {
    _size = other._size;
    _data = other._data;
    other._size = 0;
    other._data = nullptr;
}

string& string::operator=(const string& other) {
    if (this != &other) {
        delete[] _data;
        _size = other._size;
        _data = new char[_size + 1];
        strcpy(_data, other._data);
    }

    return *this;
}

string& string::operator=(string&& other) {
    if (this != &other) {
        delete[] _data;
        _size = other._size;
        _data = other._data;
        other._size = 0;
        other._data = nullptr;
    }

    return *this;
}

bool string::operator==(const char* other) const { return strcmp(_data, other) == 0; }
bool string::operator==(const string& other) const { return *this == other.c_str(); }

string& string::operator+=(const char* other) {
    size_t otherSize = strlen(other);
    char* newData = new char[_size + otherSize + 1];
    memcpy(newData, _data, _size);
    memcpy(newData + _size, other, otherSize);
    newData[_size + otherSize] = '\0';

    delete[] _data;
    _data = newData;
    _size += otherSize;

    return *this;
}

string& string::operator+=(const string& other) { return *this += other.c_str(); }

string string::operator+(const char* other) const {
    string result(*this);
    result += other;
    return result;
}

string string::operator+(const string& other) const { return *this + other.c_str(); }

string operator+(const char* lhs, const string& rhs) {
    string result(lhs);
    result += rhs;
    return result;
}

}  // namespace estd
