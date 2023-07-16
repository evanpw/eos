#pragma once
#include <stddef.h>
#include <stdint.h>

#include "estd/assertions.h"

class DiskDevice {
public:
    virtual ~DiskDevice() = default;
    virtual bool readSectors(void* dest, uint64_t start, size_t count) = 0;
    virtual size_t numSectors() const = 0;
};

class DiskPartitionDevice : public DiskDevice {
public:
    DiskPartitionDevice(DiskDevice& parent, uint64_t start, size_t numSectors)
    : _parent(parent), _partitionStart(start), _numSectors(numSectors) {
        ASSERT(start + numSectors <= parent.numSectors());
    }

    bool readSectors(void* dest, uint64_t start, size_t count) override {
        ASSERT(count < _numSectors);
        return _parent.readSectors(dest, _partitionStart + start, count);
    }

    size_t numSectors() const override { return _numSectors; }

private:
    DiskDevice& _parent;
    uint64_t _partitionStart;
    size_t _numSectors;
};
