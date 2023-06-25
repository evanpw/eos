// Defines the memory map structure return by the INT 15h, EAX=E820h function
#pragma once
#include "estd/span.h"

enum AddressRangeType : uint32_t {
    Available = 1,
    Reserved = 2,
    ACPI = 3,
    NVS = 4,
    Unusable = 5,
    Disabled = 6,
};

enum AddressRangeExtended : uint32_t {
    NonVolatile = 2,
    SlowAccess = 4,
    ErrorLog = 8,
};

struct __attribute__((packed)) E820Entry {
    uint64_t base;
    uint64_t length;
    AddressRangeType type;
    uint32_t extended;
};

using E820Table = Span<E820Entry>;
