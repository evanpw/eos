#pragma once
#include <stddef.h>
#include <stdint.h>

#include "assertions.h"
#include "bits.h"
#include "span.h"

struct __attribute__((packed)) E820Entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t extended;
};

using E820Table = Span<E820Entry>;

constexpr uint64_t KiB = 1024;
constexpr uint64_t MiB = 1024 * KiB;
constexpr uint64_t GiB = 1024 * MiB;
constexpr uint64_t PAGE_SIZE = 4 * KiB;

// Page map flags
constexpr uint64_t PAGE_PRESENT = 1 << 0;
constexpr uint64_t PAGE_WRITABLE = 1 << 1;
constexpr uint64_t PAGE_SIZE_FLAG = 1 << 7;

class PageMapEntry;
static PageMapEntry* const PML4 = reinterpret_cast<PageMapEntry*>(0x7C000);

static uint32_t* E820_NUM_ENTRIES_PTR = reinterpret_cast<uint32_t*>(0x1000);
static E820Entry* E820_TABLE = reinterpret_cast<E820Entry*>(0x1004);

inline void flushTLB() {
    asm volatile(
        "movq  %%cr3, %%rax\n\t"
        "movq  %%rax, %%cr3\n\t"
        :
        :
        : "memory", "rax");
}

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

    PageMapEntry(PhysicalAddress addr, uint64_t flags) : raw(addr.value) {
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

struct FreePageRange {
    FreePageRange(PhysicalAddress start, PhysicalAddress end)
    : start(start), end(end), next(NULL) {
        ASSERT(start.pageOffset() == 0 && end.pageOffset() == 0 && end > start);
    }

    PhysicalAddress start;
    PhysicalAddress end;

    // Intrusive linked list of free page ranges
    FreePageRange* next;
};

// Handles physical and virtual memory at the page level
class MemoryManager {
public:
    MemoryManager();
    size_t freePageCount() const;

    PhysicalAddress pageAlloc();
    void mapPage(VirtualAddress virtAddr, PhysicalAddress physAddr,
                 int pageSize = 0);

private:
    E820Table _e820Table;

    VirtualAddress physicalToVirtual(PhysicalAddress physAddr);
    void buildLinearMemoryMap(uint64_t physicalMemoryRange);
    FreePageRange* buildFreePageList();

    FreePageRange* _freePageList = nullptr;

    PhysicalAddress _linearMapEnd = 0;
    const PhysicalAddress _identityMapEnd = 2 * MiB;
    const VirtualAddress _linearMapOffset = 0xFFFF800000000000;
};
