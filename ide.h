// Drivers for an IDE controller connected to the PCI bus and for ATA and ATAPI
// devices connected to it
#pragma once
#include <stddef.h>
#include <stdint.h>

#include "disk.h"
#include "estd/memory.h"
#include "estd/print.h"

class IDEChannel;
enum class DriveSelector;

// A disk device connected to an IDE controller, either standard ATA or ATAPI
struct IDEDevice : public DiskDevice {
    friend class IDEController;

public:
    IDEDevice(IDEChannel& channel, DriveSelector drive)
    : _channel(channel), _drive(drive) {}

    virtual ~IDEDevice() = default;

    const char* modelName() const { return _modelName; }
    size_t numSectors() const override { return _numSectors; }
    virtual bool isATAPI() const = 0;

protected:
    IDEChannel& _channel;
    DriveSelector _drive;

    char _modelName[41] = {0};
    bool _lba48 = false;
    size_t _numSectors = 0;
};

// A standard (non-packet) ATA device
struct ATADevice : public IDEDevice {
    ATADevice(IDEChannel& channel, DriveSelector drive) : IDEDevice(channel, drive) {}

    bool readSectors(void* dest, uint64_t start, size_t count) override;
    bool isATAPI() const override { return false; }
};

// An ATAPI (ATA packet interface) device, usually an optical drive
struct ATAPIDevice : public IDEDevice {
    ATAPIDevice(IDEChannel& channel, DriveSelector drive) : IDEDevice(channel, drive) {}

    bool readSectors(void*, uint64_t, size_t) override {
        println("ATAPIDevice::readSectors not implemented");
        return false;
    }
    bool isATAPI() const override { return true; }
};

class IDEController {
public:
    IDEController();
    ~IDEController();

    DiskDevice& rootPartition() const {
        ASSERT(_rootPartition);
        return *_rootPartition;
    }
    IDEDevice& primaryMaster() const {
        ASSERT(_primaryMaster);
        return *_primaryMaster;
    }
    IDEDevice& primarySlave() const {
        ASSERT(_primarySlave);
        return *_primarySlave;
    }
    IDEDevice& secondaryMaster() const {
        ASSERT(_secondaryMaster);
        return *_secondaryMaster;
    }
    IDEDevice& secondarySlave() const {
        ASSERT(_secondarySlave);
        return *_secondarySlave;
    }

private:
    estd::unique_ptr<IDEDevice> detectDrive(IDEChannel& channel, DriveSelector drive);
    void detectPartitions(IDEDevice& device);

    estd::unique_ptr<IDEChannel> _primary;
    estd::unique_ptr<IDEChannel> _secondary;

    estd::unique_ptr<IDEDevice> _primaryMaster;
    estd::unique_ptr<IDEDevice> _primarySlave;
    estd::unique_ptr<IDEDevice> _secondaryMaster;
    estd::unique_ptr<IDEDevice> _secondarySlave;

    estd::unique_ptr<DiskPartitionDevice> _rootPartition;
};
