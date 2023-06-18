#pragma once
#include "estd/span.h"

struct __attribute__((packed)) E820Entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t extended;
};

using E820Table = Span<E820Entry>;
