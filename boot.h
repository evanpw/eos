#pragma once

class PageMapEntry;
static PageMapEntry* const PML4 = reinterpret_cast<PageMapEntry*>(0x7C000);

struct E820Entry;
static uint32_t* E820_NUM_ENTRIES_PTR = reinterpret_cast<uint32_t*>(0x1000);
static E820Entry* E820_TABLE = reinterpret_cast<E820Entry*>(0x1004);
