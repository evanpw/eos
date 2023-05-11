#pragma once
#include <stdint.h>

struct __attribute__ ((packed)) SMapEntry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t extended;
};

constexpr uint64_t KiB = 1024;
constexpr uint64_t MiB = 1024 * KiB;
constexpr uint64_t GiB = 1024 * MiB;
