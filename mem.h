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
constexpr uint64_t PAGE_SIZE_FLAG = 1 << 7;

void* memset(void* dest, uint8_t value, size_t n);

uint64_t lowBits(uint64_t value, int count);
uint64_t highBits(uint64_t value, int count);
uint64_t bitRange(uint64_t value, int start, int length);
uint64_t clearLowBits(uint64_t value, int count);

// TODO: move these elsewhere
template <typename T>
T min(const T& lhs, const T& rhs) {
    return (rhs < lhs) ? rhs : lhs;
}

template <typename T>
T max(const T& lhs, const T& rhs) {
    return (lhs < rhs) ? rhs : lhs;
}

inline void flushTLB() {
    asm volatile (
        "movq  %%cr3, %%rax\n\t"
        "movq  %%rax, %%cr3\n\t"
        : : : "memory", "rax"
    );
}

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

    uint64_t pageBase(int pageSize) const {
        ASSERT(pageSize >= 0 && pageSize <= 2);
        int bits = 12 + 9 * pageSize;
        return clearLowBits(value, bits);
    }

    uint64_t pageOffset(int pageSize) const {
        ASSERT(pageSize >= 0 && pageSize <= 2);
        return lowBits(value, 12 + 9 * pageSize);
    }

    uint64_t value;
};

struct VirtualAddress {
    VirtualAddress(uint64_t value) : value(value) {}

    VirtualAddress operator+(const VirtualAddress& rhs) const {
        return VirtualAddress(value + rhs.value);
    }

    uint64_t pageBase() const {
        return clearLowBits(value, 12);
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

    template <typename T>
    T* ptr() const {
        return reinterpret_cast<T*>(value);
    }

    operator void*() const { return ptr<void>(); }

    uint64_t value;
};

struct PageMapEntry {
    PageMapEntry() : raw(0) {}
    PageMapEntry(PhysicalAddress addr) : raw(addr.value) {}
    PageMapEntry(uint64_t value) : raw(value) {}

    PageMapEntry(PhysicalAddress addr, uint64_t flags)
    : raw(addr.value)
    {
        setFlags(flags);
    }

    operator bool() const { return raw != 0; }
    PhysicalAddress addr() const { return clearLowBits(raw, 12); }
    uint64_t flags() const { return lowBits(raw, 12); }

    void setFlags(uint64_t flags) {
        ASSERT(flags == lowBits(flags, 12));
        raw |= flags;
    }

    bool hasFlags(uint64_t flags) {
        ASSERT(flags == lowBits(flags, 12));
        return (raw & flags) == flags;
    }

    uint64_t raw;
};

static_assert(sizeof(PageMapEntry) == 8);

// Totally stupid bump-allocator for physical frames, used to allocate
// memory for the initial page map before anything else is set up.
struct BootstrapAllocator {
public:
    BootstrapAllocator(PhysicalAddress start, PhysicalAddress end)
    : _start(start), _end(end), _next(start)
    {}

    PhysicalAddress allocatePhysicalPage();

private:
    PhysicalAddress _start;
    PhysicalAddress _end;
    PhysicalAddress _next;
};

class MemoryManager {
public:
    MemoryManager(uint32_t numEntries, SMapEntry* smap);

    void mapPage(BootstrapAllocator& alloc, VirtualAddress virtAddr, PhysicalAddress physAddr, int pageSize = 0);

private:
    VirtualAddress physicalToVirtual(PhysicalAddress physAddr);
    void buildLinearMemoryMap(BootstrapAllocator& alloc, uint64_t physicalMemoryRange);

    PhysicalAddress _linearMapEnd = 0;
    const PhysicalAddress _identityMapEnd = 2 * MiB;
    const VirtualAddress _linearMapOffset = 0xFFFF800000000000;
};
