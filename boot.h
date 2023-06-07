#pragma once

class PageMapEntry;
static PageMapEntry* const PML4 = reinterpret_cast<PageMapEntry*>(0x7C000);

struct E820Entry;
static uint32_t* const E820_NUM_ENTRIES_PTR =
    reinterpret_cast<uint32_t*>(0x1000);
static E820Entry* const E820_TABLE = reinterpret_cast<E820Entry*>(0x1004);

struct TaskStateSegment;
static TaskStateSegment* const TSS =
    reinterpret_cast<TaskStateSegment*>(0x7C00 - 0x70);

enum SegmentSelector : uint16_t {
    SELECTOR_CODE0 = 0x08,
    SELECTOR_DATA0 = 0x10,
    SELECTOR_DATA3 = 0x20,
    SELECTOR_CODE3 = 0x28,
    SELECTOR_TSS = 0x30,
};
