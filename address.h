// Defines classes to represent physical and virtual memory addresses, mainly
// to distinguish one from another in places where it matter (e.g., paging
// infrastructure)
#pragma once
#include <stddef.h>
#include <stdint.h>

#include "estd/assertions.h"
#include "estd/bits.h"
#include "units.h"

struct PhysicalAddress {
    PhysicalAddress(uint64_t value = 0) : value(value) {}

    PhysicalAddress operator+(size_t delta) const {
        return PhysicalAddress(value + delta);
    }

    PhysicalAddress& operator+=(size_t delta) {
        // TODO: check for overflow
        value += delta;
        return *this;
    }

    int64_t operator-(const PhysicalAddress& rhs) const { return value - rhs.value; }

    bool operator<(const PhysicalAddress& rhs) const { return value < rhs.value; }
    bool operator<=(const PhysicalAddress& rhs) const { return value <= rhs.value; }
    bool operator>(const PhysicalAddress& rhs) const { return value > rhs.value; }
    bool operator>=(const PhysicalAddress& rhs) const { return value >= rhs.value; }
    bool operator==(const PhysicalAddress& rhs) const { return value == rhs.value; }

    uint64_t pageBase(int pageSize = 0) const {
        ASSERT(pageSize >= 0 && pageSize <= 2);
        int bits = 12 + 9 * pageSize;
        return clearLowBits(value, bits);
    }

    uint64_t pageOffset(int pageSize = 0) const {
        ASSERT(pageSize >= 0 && pageSize <= 2);
        return lowBits(value, 12 + 9 * pageSize);
    }

    uint64_t pageFrameIdx() {
        ASSERT(pageOffset() == 0);
        return value / PAGE_SIZE;
    }

    uint64_t value;
};

static_assert(sizeof(PhysicalAddress) == sizeof(uint64_t));

struct VirtualAddress {
    VirtualAddress(uint64_t value = 0) : value(value) {}

    VirtualAddress operator+(int64_t delta) const {
        return VirtualAddress(value + delta);
    }
    VirtualAddress operator-(int64_t delta) const {
        return VirtualAddress(value - delta);
    }

    VirtualAddress& operator+=(int64_t delta) {
        // TODO: check for overflow
        value += delta;
        return *this;
    }

    VirtualAddress& operator-=(int64_t delta) {
        // TODO: check for overflow
        value -= delta;
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

static_assert(sizeof(VirtualAddress) == sizeof(uint64_t));
