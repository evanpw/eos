#include "mem.h"
#include "assertions.h"
#include "print.h"

// TODO: this is really slow
void* memset(void* dest, uint8_t value, size_t n) {
    uint8_t* ptr = static_cast<uint8_t*>(dest);

    for (size_t i = 0; i < n; ++i) {
        *ptr++ = value;
    }

    return dest;
}

uint64_t lowBits(uint64_t value, int count) {
    ASSERT(count >= 0 && count <= 64);
    return value & ((1 << count) - 1);
}

uint64_t highBits(uint64_t value, int count) {
    ASSERT(count >= 0 && count <= 64);
    return value >> (64 - count);
}

uint64_t bitRange(uint64_t value, int start, int length) {
    ASSERT(start >= 0 && start <= 64 && length >= 0 && start + length <= 64);
    return lowBits(highBits(value, 64 - start), length);
}

uint64_t clearLowBits(uint64_t value, int count) {
    ASSERT(count >= 0 && count <= 64);
    return value & ~((1 << count) - 1);
}

PhysicalAddress BootstrapAllocator::allocatePhysicalPage()
{
    ASSERT(_next < _end);

    PhysicalAddress result = _next;
    _next += PAGE_SIZE;
    return result;
}

MemoryManager::MemoryManager(uint32_t numEntries, SMapEntry* smap) {
    uint64_t availableBytes = 0;
    uint64_t physicalMemoryRange = 0;
    uint64_t availableAt1MiB = 0;

    // TODO: check for overlapping entries
    for (uint32_t i = 0; i < numEntries; ++i) {
        const auto& entry = smap[i];
        uint64_t end = entry.base + entry.length - 1;
        physicalMemoryRange = max(physicalMemoryRange, end + 1);

        if (entry.type != 1) {
            continue;
        }

        availableBytes += entry.length;

        if (entry.base <= 1 * MiB && end >= 1 * MiB) {
            availableAt1MiB = end - 1 * MiB + 1;
        }

    }

    println("Available memory: {}MiB", availableBytes / MiB);

    BootstrapAllocator alloc(1 * MiB, 1 * MiB + availableAt1MiB);
    buildLinearMemoryMap(alloc, physicalMemoryRange);
}

VirtualAddress MemoryManager::physicalToVirtual(PhysicalAddress physAddr) {
    if (physAddr < _linearMapEnd) {
        return _linearMapOffset + physAddr.value;
    }

    ASSERT(physAddr < _identityMapEnd);
    return VirtualAddress(physAddr.value);
}

void MemoryManager::buildLinearMemoryMap(BootstrapAllocator& alloc, uint64_t physicalMemoryRange) {
    // Attempt to map all of physical memory to high virtual memory
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

        mapPage(alloc, _linearMapOffset + current, PhysicalAddress(current), pageSize);

        current += PAGE_SIZE << (9 * pageSize);
        _linearMapEnd = PhysicalAddress(current);
    }
}

void MemoryManager::mapPage(BootstrapAllocator& alloc, VirtualAddress virtAddr, PhysicalAddress physAddr, int pageSize) {
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

    // The pointer to the current page map level (PML4, PDP, PD, PT), starting with PML4
    PageMapEntry* pml = reinterpret_cast<PageMapEntry*>(0x7C000);

    // Traverse the PMLs in order (PML4, PDP, PD)
    for (int n = 4; n > pageSize + 1; --n) {
        uint16_t index = virtAddr.pageMapIndex(n);

        PageMapEntry entry = pml[index];
        if (!entry) {
            // No existing entry in the current PML

            // Create a new empty next PML in fresh physical memory
            PhysicalAddress pmlNextPhysAddr = alloc.allocatePhysicalPage();
            void* pmlNext = physicalToVirtual(pmlNextPhysAddr);
            memset(pmlNext, 0, PAGE_SIZE);

            // Point the correct entry in the PML4 to the new PDP and mark it present
            // and writable
            entry = PageMapEntry(pmlNextPhysAddr, PAGE_PRESENT | PAGE_WRITABLE);
            pml[index] = entry;
        } else {
            ASSERT(entry.hasFlags(PAGE_PRESENT | PAGE_WRITABLE));
        }

        // Advance to the next level
        pml = physicalToVirtual(entry.addr()).ptr<PageMapEntry>();
    }

    // After reaching this point, pml is a pointer to the correct page table (or higher page map,
    // if using large pages)
    uint16_t index = virtAddr.pageMapIndex(pageSize + 1);

    // We can't remap existing pages yet
    ASSERT(!pml[index]);

    PageMapEntry entry(physAddr, PAGE_PRESENT | PAGE_WRITABLE);
    if (pageSize > 0) {
        entry.setFlags(PAGE_SIZE_FLAG);
    }

    pml[index] = entry;
}
