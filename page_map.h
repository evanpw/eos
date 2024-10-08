// Defines the structures and functions used in managing page tables and
// address spaces
#pragma once
#include <stdint.h>

#include "address.h"
#include "boot.h"
#include "estd/bits.h"
#include "estd/memory.h"
#include "estd/vector.h"
#include "units.h"

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

class MemoryManager;
class UserAddressSpace;

class KernelAddressSpace {
public:
    KernelAddressSpace(MemoryManager& mm) : _mm(mm) {}

    void mapPage(VirtualAddress virtAddr, PhysicalAddress physAddr, int pageSize = 0,
                 uint64_t flags = 0);

    VirtualAddress physicalToVirtual(PhysicalAddress physAddr);
    PhysicalAddress virtualToPhysical(VirtualAddress virtAddr);

    estd::unique_ptr<UserAddressSpace> makeUserAddressSpace();

private:
    MemoryManager& _mm;
    const PhysicalAddress _pml4 = KERNEL_PML4;

    const PhysicalAddress _identityMapEnd = 2 * MiB;
    const VirtualAddress _linearMapOffset = 0xFFFF800000000000;
    PhysicalAddress _linearMapEnd = 0;

    friend class MemoryManager;
    void buildLinearMemoryMap(uint64_t physicalMemoryRange);
};

class UserAddressSpace {
    friend class KernelAddressSpace;

public:
    ~UserAddressSpace();

    void mapPage(VirtualAddress virtAddr, PhysicalAddress physAddr, int pageSize = 0);
    void mapPages(VirtualAddress virtAddr, PhysicalAddress physAddr, size_t count);

    PhysicalAddress pml4() const { return _pml4; }
    VirtualAddress userMapBase() const { return _userMapBase; }

    VirtualAddress vmalloc(size_t pageCount);

private:
    UserAddressSpace(KernelAddressSpace& kaddr, PhysicalAddress pml4);

    KernelAddressSpace& _kaddr;
    PhysicalAddress _pml4;

    // Usermode virtual addresses start at 512GiB (pml4[1])
    const VirtualAddress _userMapBase = 0x8000000000;

    // Simple bump-pointer virtual memory allocator
    VirtualAddress _nextUserAddress = _userMapBase + 2 * MiB;

    // Keep track of the pages we've used for PMLs so that we can free them at destructor
    // time
    estd::vector<PhysicalAddress> _allocatedPages;
};
