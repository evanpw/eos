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

void* BootstrapAllocator::allocatePhysicalPage()
{
    ASSERT(_next < _end);

    void* result = reinterpret_cast<void*>(_next.value);
    _next += 4 * KiB;
    return result;
}

void explainVirtualAddress(VirtualAddress virtAddr)
{
    println("virtAddr = 0x{:X}", virtAddr.value);

    // Break down the virtual address into its components

    // The top 18 bits must be all 1 or all zero (canonical form)
    ASSERT(virtAddr.isCanonical());

    // The remaining bits are offsets into the page maps
    uint16_t index1 = virtAddr.pageMapIndex(1);
    uint16_t index2 = virtAddr.pageMapIndex(2);
    uint16_t index3 = virtAddr.pageMapIndex(3);
    uint16_t index4 = virtAddr.pageMapIndex(4);
    println("index1={}, index2={}, index3={}, index4={}", index1, index2, index3, index4);

    // Determine how this address is currently mapped

    // Level 4
    uint64_t* pml4 = reinterpret_cast<uint64_t*>(0x7C000);
    uint64_t entry4 = pml4[index4];
    if (entry4 == 0) {
        println("pml4[index4] = 0");
        return;
    }

    uint64_t* pdp = reinterpret_cast<uint64_t*>(clearLowBits(entry4, 12));
    println("pml4[index4] = 0x{:X}, present={}, writable={}, pdp=0x{:X}",
        entry4,
        entry4 & 1,
        (entry4 >> 1) & 1,
        reinterpret_cast<uint64_t>(pdp)
    );

    // All of the page maps have to be in identity-mapped sub-2MiB memory at this point
    ASSERT(pdp < reinterpret_cast<uint64_t*>(2 * MiB));

    // Level 3
    uint64_t entry3 = pdp[index3];
    if (entry3 == 0) {
        println("pdp[index3] = 0");
        return;
    }

    uint64_t* pd = reinterpret_cast<uint64_t*>(clearLowBits(entry3, 12));
    println("pdp[index3] = 0x{:X}, present={}, writable={}, pd=0x{:X}",
        entry3,
        entry3 & 1,
        (entry3 >> 1) & 1,
        reinterpret_cast<uint64_t>(pd)
    );

    // All of the page maps have to be in identity-mapped sub-2MiB memory at this point
    ASSERT(pd < reinterpret_cast<uint64_t*>(2 * MiB));

    // Level 2
    uint64_t entry2 = pd[index2];
    if (entry2 == 0) {
        println("pml2[index2] = 0");
        return;
    }

    uint64_t* pt = reinterpret_cast<uint64_t*>(clearLowBits(entry2, 12));
    println("pt[index2] = 0x{:X}, present={}, writable={}, pt=0x{:X}",
        entry2,
        entry2 & 1,
        (entry2 >> 1) & 1,
        reinterpret_cast<uint64_t>(pt)
    );

    // All of the page maps have to be in identity-mapped sub-2MiB memory at this point
    ASSERT(pt < reinterpret_cast<uint64_t*>(2 * MiB));

    // Level 1
    uint64_t entry1 = pt[index1];
    if (entry1 == 0) {
        println("pt[index1] = 0");
        return;
    }

    uint64_t* physAddr = reinterpret_cast<uint64_t*>(clearLowBits(entry1, 12));
    println("pt[index1] = 0x{:X}, present={}, writable={}, physAddr=0x{:X}",
        entry1,
        entry1 & 1,
        (entry1 >> 1) & 1,
        reinterpret_cast<uint64_t>(physAddr)
    );
}

void mapPage(BootstrapAllocator& alloc, VirtualAddress virtAddr, PhysicalAddress physAddr) {
    // Break down the virtual address into its components

    // The top 18 bits must be all 1 or all zero (canonical form)
    ASSERT(virtAddr.isCanonical());

    // The bottom 12 bits must be zero (page-aligned)
    ASSERT(virtAddr.pageOffset() == 0);

    // The pointer to the current page map level (PML4, PDP, PD, PT), starting with PML4
    uint64_t* pml = reinterpret_cast<uint64_t*>(0x7C000);

    // Traverse the first 3 PMLs in order (PML4, PDP, PD)
    for (int n = 4; n > 1; --n) {
        uint16_t index = virtAddr.pageMapIndex(n);

        uint64_t entry = pml[index];
        if (entry != 0) {
            // The page containing the next PML should always be present and writable
            ASSERT((entry & PAGE_PRESENT) && (entry & PAGE_WRITABLE));

            // Masking out the flag bits gives us the physical address of the next PML
            uint64_t pmlNextPhysAddr = clearLowBits(entry, 12);

            // At this stage, the next PML should be in identity-mapped sub-2MiB memory, so
            // the virtual address is the same
            ASSERT(pmlNextPhysAddr < 2 * MiB);
        } else {
            // No existing entry in the current PML

            // Create a new empty next PML in fresh physical memory
            uint64_t* pmlNext = static_cast<uint64_t*>(alloc.allocatePhysicalPage());
            memset(pmlNext, 0, PAGE_SIZE);

            // Point the correct entry in the PML4 to the new PDP and mark it present
            // and writable
            entry = reinterpret_cast<uint64_t>(pmlNext) | PAGE_PRESENT | PAGE_WRITABLE;
            pml[index] = entry;
        }

        // Advance to the next level
        pml = reinterpret_cast<uint64_t*>(clearLowBits(entry, 12));
    }

    // After reaching this point, pml is a pointer to the correct page table
    uint16_t index = virtAddr.pageMapIndex(1);

    // All of the page maps have to be in identity-mapped sub-2MiB memory at this point
    ASSERT(pml < reinterpret_cast<uint64_t*>(2 * MiB));

    // Level 1
    uint64_t entry = pml[index];

    // We can't remap existing pages yet
    ASSERT(entry == 0);
    pml[index] = physAddr.value | PAGE_PRESENT | PAGE_WRITABLE;

    // We don't need to flush the TLB when mapping a previously-unmapped virtual page,
    // because only "present" pages reside in the cache (AMD Vol 3A, 4.10.2.3)
}
