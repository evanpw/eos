#include "page_map.h"

#include <string.h>

#include "mm.h"
#include "system.h"

// Page map flags
constexpr uint64_t PAGE_PRESENT = 1 << 0;
constexpr uint64_t PAGE_WRITABLE = 1 << 1;
constexpr uint64_t PAGE_USER = 1 << 2;
constexpr uint64_t PAGE_SIZE_FLAG = 1 << 7;

template <typename F>
static void mapPageImpl(MemoryManager& mm, PhysicalAddress pml4, VirtualAddress virtAddr,
                        PhysicalAddress physAddr, int pageSize, uint64_t flags,
                        F&& allocCallback) {
    // pageSize = 0: 4KiB pages
    // pageSize = 1: 2MiB pages
    // pageSize = 2: 1GiB pages
    ASSERT(pageSize >= 0 && pageSize <= 2);

    // The top 18 bits must be all 1 or all zero (canonical form)
    ASSERT(virtAddr.isCanonical());

    // The physical address must be aligned to the page size
    ASSERT(physAddr.pageOffset(pageSize) == 0);

    // The virtual address must be page-aligned
    ASSERT(virtAddr.pageOffset() == 0);

    // The pointer to the current page map level (PML4, PDP, PD, PT), starting
    // with PML4
    PageMapEntry* pml = mm.physicalToVirtual(pml4).ptr<PageMapEntry>();

    // Traverse the PMLs in order (PML4, PDP, PD)
    for (int n = 4; n > pageSize + 1; --n) {
        uint16_t index = virtAddr.pageMapIndex(n);

        PageMapEntry entry = pml[index];
        if (!entry) {
            // No existing entry in the current PML

            // Create a new empty next PML in fresh physical memory
            PhysicalAddress pmlNextPhysAddr = mm.pageAlloc();
            allocCallback(pmlNextPhysAddr);

            // Point the correct entry in the PML4 to the new PDP and mark it
            // present and writable
            entry = PageMapEntry(pmlNextPhysAddr, PAGE_PRESENT | PAGE_WRITABLE | flags);
            pml[index] = entry;
        } else {
            ASSERT(entry.hasFlags(PAGE_PRESENT | PAGE_WRITABLE | flags));
        }

        // Advance to the next level
        pml = mm.physicalToVirtual(entry.addr()).ptr<PageMapEntry>();
    }

    // After reaching this point, pml is a pointer to the correct page table (or
    // higher page map, if using large pages)
    uint16_t index = virtAddr.pageMapIndex(pageSize + 1);

    // We can't remap existing pages yet
    ASSERT(!pml[index]);

    PageMapEntry entry(physAddr, PAGE_PRESENT | PAGE_WRITABLE | flags);
    if (pageSize > 0) {
        entry.setFlags(PAGE_SIZE_FLAG);
    }

    pml[index] = entry;
}

void KernelAddressSpace::mapPage(VirtualAddress virtAddr, PhysicalAddress physAddr,
                                 int pageSize, uint64_t flags) {
    mapPageImpl(_mm, _pml4, virtAddr, physAddr, pageSize, flags, [](PhysicalAddress) {});
}

VirtualAddress KernelAddressSpace::physicalToVirtual(PhysicalAddress physAddr) {
    if (physAddr < _linearMapEnd) {
        return _linearMapOffset + physAddr.value;
    }

    ASSERT(physAddr < _identityMapEnd);
    return VirtualAddress(physAddr.value);
}

PhysicalAddress KernelAddressSpace::virtualToPhysical(VirtualAddress physAddr) {
    if (physAddr < _linearMapOffset ||
        physAddr >= _linearMapOffset + _linearMapEnd.value) {
        return 0;
    }

    return PhysicalAddress(physAddr.value - _linearMapOffset.value);
}

void KernelAddressSpace::buildLinearMemoryMap(uint64_t physicalMemoryRange) {
    // Linearly map all physical memory to high virtual memory
    uint64_t current = 0;
    while (current < physicalMemoryRange) {
        int pageSize;
        if (current % GiB == 0 && physicalMemoryRange - current >= GiB) {
            pageSize = 2;
        } else if (current % (2 * MiB) == 0 && physicalMemoryRange - current >= 2 * MiB) {
            pageSize = 1;
        } else {
            pageSize = 0;
        }

        mapPage(_linearMapOffset + current, PhysicalAddress(current), pageSize);

        current += PAGE_SIZE << (9 * pageSize);
        _linearMapEnd = PhysicalAddress(current);
    }
}

estd::unique_ptr<UserAddressSpace> KernelAddressSpace::makeUserAddressSpace() {
    // Create a fresh empty PML4
    PhysicalAddress upml4 = _mm.pageAlloc();

    // Every user process shares the kernel page mappings
    PageMapEntry* kptr = physicalToVirtual(_pml4).ptr<PageMapEntry>();
    PageMapEntry* uptr = physicalToVirtual(upml4).ptr<PageMapEntry>();
    memcpy(uptr, kptr, 512 * sizeof(PageMapEntry));

    // Make sure that the kernel didn't map any user addresses
    for (size_t i = 1; i < _linearMapOffset.pageMapIndex(4); ++i) {
        ASSERT(uptr[i] == 0);
    }

    return estd::unique_ptr<UserAddressSpace>(new UserAddressSpace(*this, upml4));
}

UserAddressSpace::UserAddressSpace(KernelAddressSpace& kaddr, PhysicalAddress pml4)
: _kaddr(kaddr), _pml4(pml4) {
    _allocatedPages.push_back(_pml4);
}

UserAddressSpace::~UserAddressSpace() {
    for (PhysicalAddress page : _allocatedPages) {
        mm.pageFree(page);
    }
}

void UserAddressSpace::mapPage(VirtualAddress virtAddr, PhysicalAddress physAddr,
                               int pageSize) {
    mapPageImpl(mm, _pml4, virtAddr, physAddr, pageSize, PAGE_USER,
                [this](PhysicalAddress page) { _allocatedPages.push_back(page); });
}

void UserAddressSpace::mapPages(VirtualAddress virtAddr, PhysicalAddress physAddr,
                                size_t count) {
    // TODO: for large allocations we can use larger pages
    for (size_t i = 0; i < count; ++i) {
        mapPage(virtAddr + i * PAGE_SIZE, physAddr + i * PAGE_SIZE);
    }
}

VirtualAddress UserAddressSpace::vmalloc(size_t pageCount) {
    VirtualAddress virtAddr = _nextUserAddress;
    _nextUserAddress += pageCount * PAGE_SIZE;
    return virtAddr;
}
