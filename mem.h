#pragma once
#include <stddef.h>
#include <stdint.h>

#include "address.h"
#include "assertions.h"
#include "bits.h"
#include "boot.h"
#include "e820.h"
#include "page_map.h"
#include "span.h"
#include "units.h"

// Configuration
constexpr size_t HEAP_SIZE = 2 * MiB;

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
    size_t freePageCount() const;

    PhysicalAddress pageAlloc(size_t count = 1);

    VirtualAddress physicalToVirtual(PhysicalAddress physAddr) {
        return _kaddressSpace.physicalToVirtual(physAddr);
    }

    KernelAddressSpace& kaddressSpace() { return _kaddressSpace; }

    void* kmalloc(size_t size);
    void kfree(void* ptr);
    void showHeap() const;

private:
    friend class System;
    MemoryManager();

    E820Table _e820Table;
    KernelAddressSpace _kaddressSpace;

    FreePageRange* _freePageList = nullptr;
    FreePageRange* buildFreePageList();

    struct __attribute__((packed)) BlockHeader {
        static BlockHeader freeBlock(uint32_t size) {
            ASSERT(size % 2 == 0);
            return BlockHeader{size};
        }

        static BlockHeader usedBlock(uint32_t size) {
            ASSERT(size % 2 == 0);
            return BlockHeader{size | 1};
        }

        uint32_t size() const { return clearLowBits(raw, 1); }
        bool isFree() const { return lowBits(raw, 1) == 0; }

        uint32_t raw = 0;
    };

    static_assert(sizeof(BlockHeader) == 4);

    uint8_t* _heap = nullptr;
    void initializeHeap();
};
