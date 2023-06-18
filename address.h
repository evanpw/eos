#pragma once
#include <stdint.h>

#include "estd/bits.h"

struct PhysicalAddress {
    PhysicalAddress(uint64_t value) : value(value) {}

    PhysicalAddress operator+(size_t delta) const {
        return PhysicalAddress(value + delta);
    }

    PhysicalAddress& operator+=(size_t delta) {
        // TODO: check for overflow
        value += delta;
        return *this;
    }

    int64_t operator-(const PhysicalAddress& rhs) const {
        return value - rhs.value;
    }

    bool operator<(const PhysicalAddress& rhs) const {
        return value < rhs.value;
    }
    bool operator<=(const PhysicalAddress& rhs) const {
        return value <= rhs.value;
    }
    bool operator>(const PhysicalAddress& rhs) const {
        return value > rhs.value;
    }
    bool operator>=(const PhysicalAddress& rhs) const {
        return value >= rhs.value;
    }
    bool operator==(const PhysicalAddress& rhs) const {
        return value == rhs.value;
    }

    uint64_t pageBase(int pageSize = 0) const {
        ASSERT(pageSize >= 0 && pageSize <= 2);
        int bits = 12 + 9 * pageSize;
        return clearLowBits(value, bits);
    }

    uint64_t pageOffset(int pageSize = 0) const {
        ASSERT(pageSize >= 0 && pageSize <= 2);
        return lowBits(value, 12 + 9 * pageSize);
    }

    uint64_t value;
};

struct VirtualAddress {
    VirtualAddress(uint64_t value) : value(value) {}

    VirtualAddress operator+(size_t delta) const {
        return VirtualAddress(value + delta);
    }

    VirtualAddress& operator+=(size_t delta) {
        // TODO: check for overflow
        value += delta;
        return *this;
    }

    uint64_t pageBase() const { return clearLowBits(value, 12); }

    uint64_t pageOffset() const { return lowBits(value, 12); }

    bool isCanonical() const {
        // The top 17 bits must be all 1 or all zero
        return bitSlice(value, 47) == 0 || bitSlice(~value, 47) == 0;
    }

    uint16_t pageMapIndex(int level) const {
        ASSERT(level >= 1 && level <= 4);
        return bitRange(value, 9 * (level - 1) + 12, 9);
    }

    template <typename T>
    T* ptr() const {
        return reinterpret_cast<T*>(value);
    }

    operator void*() const { return ptr<void>(); }

    uint64_t value;
};
