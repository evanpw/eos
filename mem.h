#pragma once
#include <stddef.h>
#include <stdint.h>
#include "assertions.h"

struct __attribute__ ((packed)) SMapEntry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t extended;
};

constexpr uint64_t KiB = 1024;
constexpr uint64_t MiB = 1024 * KiB;
constexpr uint64_t GiB = 1024 * MiB;
constexpr uint64_t PAGE_SIZE = 4 * KiB;

// Page map flags
constexpr uint64_t PAGE_PRESENT = 1 << 0;
constexpr uint64_t PAGE_WRITABLE = 1 << 1;

void* memset(void* dest, uint8_t value, size_t n);

uint64_t lowBits(uint64_t value, int count);
uint64_t highBits(uint64_t value, int count);
uint64_t bitRange(uint64_t value, int start, int length);
uint64_t clearLowBits(uint64_t value, int count);

struct PhysicalAddress {
    PhysicalAddress(uint64_t value) : value(value) {}

    PhysicalAddress operator+(const PhysicalAddress& rhs) const {
        return PhysicalAddress(value + rhs.value);
    }

    PhysicalAddress& operator+=(uint64_t delta) {
        // TODO: check for overflow
        value += delta;
        return *this;
    }

    bool operator<(const PhysicalAddress& rhs) const { return value < rhs.value; }

    uint64_t value;
};

struct VirtualAddress {
    VirtualAddress(uint64_t value) : value(value) {}

    VirtualAddress operator+(const VirtualAddress& rhs) const {
        return VirtualAddress(value + rhs.value);
    }

    uint64_t pageBase() const {
        return (value >> 12) << 12;
    }

    uint64_t pageOffset() const {
        return lowBits(value, 12);
    }

    bool isCanonical() const {
        // The top 17 bits must be all 1 or all zero
        return highBits(value, 17) == 0 || highBits(~value, 17) == 0;
    }

    uint16_t pageMapIndex(int level) const {
        ASSERT(level >= 1 && level <= 4);
        return bitRange(value, 9 * (level - 1) + 12, 9);
    }

    uint64_t value;
};

// Totally stupid bump-allocator for physical frames, used to allocate
// memory for the initial page map before anything else is set up.
struct BootstrapAllocator {
public:
    BootstrapAllocator(PhysicalAddress start, PhysicalAddress end)
    : _start(start), _end(end), _next(start)
    {}

    void* allocatePhysicalPage();

private:
    PhysicalAddress _start;
    PhysicalAddress _end;
    PhysicalAddress _next;
};

void explainVirtualAddress(VirtualAddress virtAddr);

void mapPage(BootstrapAllocator& alloc, VirtualAddress virtAddr, PhysicalAddress physAddr);
