#include "mem.h"

#include <string.h>

#include "estd/assertions.h"
#include "estd/new.h"
#include "estd/print.h"
#include "klibc.h"
#include "page_map.h"
#include "panic.h"

MemoryManager::MemoryManager()
: _e820Table(E820_TABLE, *E820_NUM_ENTRIES_PTR), _kaddressSpace(*this) {
    uint64_t availableBytes = 0;
    uint64_t physicalMemoryRange = 0;
    uint64_t availableAt1MiB = 0;

    // Count available physical memory, and find the contiguous range of
    // physical memory starting at 1MiB (we have to start there because only the
    // first 2MiB is identity-mapped by the bootloader)
    for (const auto& entry : _e820Table) {
        print("mem: {:08X}:{:08X} (", entry.base, entry.base + entry.length);
        switch (entry.type) {
            case AddressRangeType::Available:
                print("available");
                break;

            case AddressRangeType::ACPI:
                print("acpi reclaimable");
                break;

            case AddressRangeType::NVS:
                print("nvs");
                break;

            case AddressRangeType::Unusable:
                print("unusable");
                break;

            case AddressRangeType::Disabled:
                print("disabled");
                break;

            case AddressRangeType::Reserved:
            default:
                print("reserved");
                break;
        }

        if (entry.extended & AddressRangeExtended::NonVolatile) {
            print(", non-volatile");
        }

        if (entry.extended & AddressRangeExtended::SlowAccess) {
            print(", slow");
        }

        if (entry.extended & AddressRangeExtended::ErrorLog) {
            print(", error-log");
        }

        println(")");

        // TODO: check for overlapping entries
        uint64_t end = entry.base + entry.length;
        physicalMemoryRange = max(physicalMemoryRange, end);

        if (entry.type != AddressRangeType::Available) {
            continue;
        }

        availableBytes += entry.length;

        if (entry.base <= 1 * MiB && end > 1 * MiB) {
            // Round down to page-multiple size
            end = (end / PAGE_SIZE) * PAGE_SIZE;
            availableAt1MiB = end - 1 * MiB;
        }
    }

    FreePageRange initialPages(1 * MiB, 1 * MiB + availableAt1MiB);
    _freePageList = &initialPages;

    _kaddressSpace.buildLinearMemoryMap(physicalMemoryRange);
    _freePageList = buildFreePageList();

    initializeHeap();

    println("Memory manager initialized");
    println("Available physical memory: {} MiB", availableBytes / MiB);
}

PhysicalAddress MemoryManager::pageAlloc(size_t count) {
    // Linear scan, first-fit
    FreePageRange* prev = nullptr;
    FreePageRange* current = _freePageList;
    while (current) {
        uint64_t sizeInPages = (current->end - current->start) / PAGE_SIZE;
        if (sizeInPages >= count) {
            PhysicalAddress result = current->start;
            current->start += count * PAGE_SIZE;

            if (current->start == current->end) {
                FreePageRange* next = current->next;
                if (prev) {
                    prev->next = next;
                } else {
                    _freePageList = next;
                }
            }

            // Zero out the page
            void* ptr = physicalToVirtual(result);
            memset(ptr, 0, count * PAGE_SIZE);

            return result;
        }

        prev = current;
        current = current->next;
    }

    panic("OOM in MemoryManager::pageAlloc");
}

FreePageRange* MemoryManager::buildFreePageList() {
    // Assume that previous steps didn't take up the entire contiguous chunk at
    // 1MiB, and that we can store all of the initial free page ranges in one
    // page
    VirtualAddress slabStart = physicalToVirtual(pageAlloc());
    VirtualAddress slabEnd = slabStart + PAGE_SIZE;

    FreePageRange* head = nullptr;
    FreePageRange* tail = nullptr;
    for (const auto& entry : _e820Table) {
        // TODO: check for overlapping entries
        if (entry.type != AddressRangeType::Available || entry.extended != 1) {
            continue;
        }

        uint64_t base = entry.base;
        uint64_t end = entry.base + entry.length;

        // Reserve memory below 1MiB
        if (base < 1 * MiB) {
            if (end <= 1 * MiB) {
                continue;
            } else {
                base = 1 * MiB;
            }
        }

        // Align this range to page boundaries
        base = PAGE_SIZE * ((base + PAGE_SIZE - 1) / PAGE_SIZE);
        end = PAGE_SIZE * (end / PAGE_SIZE);
        if (base >= end) {
            continue;
        }

        // Allocate space for the new range
        ASSERT(slabStart + sizeof(FreePageRange) <= slabEnd);
        void* newAddr = slabStart.ptr<void>();
        slabStart += sizeof(FreePageRange);

        // Initialize the new range at the appropriate address
        FreePageRange* newRange = new (newAddr) FreePageRange(base, end);

        // Insert it into the list
        if (head) {
            tail->next = newRange;
            tail = newRange;
        } else {
            head = tail = newRange;
        }
    }

    // Mark the memory we've already used during MM initialization as not free
    ASSERT(_freePageList && !_freePageList->next);

    FreePageRange* current = head;
    while (current) {
        ASSERT(current->start >= PhysicalAddress(1 * MiB));

        if (current->start <= _freePageList->start) {
            current->start = _freePageList->start;
            ASSERT(current->start < current->end);
        }

        current = current->next;
    }

    return head;
}

size_t MemoryManager::freePageCount() const {
    size_t freePages = 0;

    FreePageRange* current = _freePageList;
    while (current) {
        ASSERT(current->start.pageOffset() == 0 &&
               current->end.pageOffset() == 0);
        freePages += (current->end - current->start) / PAGE_SIZE;
        current = current->next;
    }

    return freePages;
}

void MemoryManager::initializeHeap() {
    // Allocate and zero out a contiguous region to use as a heap
    PhysicalAddress physicalPages = pageAlloc(HEAP_SIZE / PAGE_SIZE);
    _heap = physicalToVirtual(physicalPages).ptr<uint8_t>();

    // Initialize the entire space as a single free block
    BlockHeader* firstBlock = new (_heap) BlockHeader;
    *firstBlock = BlockHeader::freeBlock(HEAP_SIZE);
}

// TODO: this should return 16-byte aligned pointers to conform with the C spec
void* MemoryManager::kmalloc(size_t size) {
    // Round up to the nearest multiple of 4 bytes
    size = 4 * ((size + 3) / 4);

    size_t requiredSize = size + sizeof(BlockHeader);

    // Linear scan first-fit
    uint8_t* ptr = _heap;
    while (ptr < _heap + HEAP_SIZE) {
        // TODO: use memcpy
        BlockHeader* header = reinterpret_cast<BlockHeader*>(ptr);

        // Skip used blocks
        if (!header->isFree()) {
            ptr += header->size();
            continue;
        }

        // Coalesce free blocks
        while (true) {
            uint8_t* nextPtr = ptr + header->size();
            BlockHeader* nextHeader = reinterpret_cast<BlockHeader*>(nextPtr);

            if (nextPtr >= _heap + HEAP_SIZE || !nextHeader->isFree()) {
                break;
            }

            // If the next block is also free, combine them
            *header =
                BlockHeader::freeBlock(header->size() + nextHeader->size());

            // Continue until we reach the end of the heap or a used block
        }

        if (header->size() >= requiredSize) {
            int64_t extraSize = header->size() - requiredSize;

            // Split block into two pieces if possible
            if (extraSize >= 4 + sizeof(BlockHeader)) {
                BlockHeader* nextHeader =
                    reinterpret_cast<BlockHeader*>(ptr + requiredSize);
                *nextHeader = BlockHeader::freeBlock(extraSize);
                *header = BlockHeader::usedBlock(requiredSize);
            } else {
                *header = BlockHeader::usedBlock(header->size());
            }

            return ptr + sizeof(BlockHeader);
        }

        ptr += header->size();
    }

    panic("OOM in MemoryManager::kmalloc");
}

void MemoryManager::kfree(void* ptr) {
    // Locate the header for this heap block and verify that it looks good
    uint8_t* blockPtr = reinterpret_cast<uint8_t*>(ptr) - sizeof(BlockHeader);
    BlockHeader* header = reinterpret_cast<BlockHeader*>(blockPtr);
    ASSERT(blockPtr >= _heap && blockPtr + header->size() <= _heap + HEAP_SIZE);
    ASSERT(header->size() % 4 == 0);
    ASSERT(header->size() >= sizeof(BlockHeader) + 4);
    ASSERT(!header->isFree());

    // Mark it as free
    *header = BlockHeader::freeBlock(header->size());
}

void MemoryManager::showHeap() const {
    uint8_t* ptr = _heap;
    while (ptr < _heap + HEAP_SIZE) {
        // TODO: use memcpy
        const BlockHeader* header = reinterpret_cast<const BlockHeader*>(ptr);
        println("addr={:X}, size={}, free={}", ptr, header->size(),
                header->isFree());

        ptr += header->size();
    }
}
