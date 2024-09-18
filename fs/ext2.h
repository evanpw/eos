#pragma once
#include "disk.h"
#include "estd/buffer.h"
#include "estd/memory.h"
#include "fs/ext2_defs.h"
#include "sys/types.h"

class Ext2FileSystem {
public:
    // Create using a static method because creation can fail
    static estd::unique_ptr<Ext2FileSystem> create(DiskDevice& disk);
    ~Ext2FileSystem();

    // High-level interface
    estd::unique_ptr<ext2::Inode> lookup(const char* path);
    bool readFullFile(const ext2::Inode& inode, uint8_t* dest);
    ssize_t readFromFile(const ext2::Inode& inode, uint8_t* dest, uint32_t size,
                         uint32_t offset = 0);

private:
    Ext2FileSystem(DiskDevice& disk);
    bool init();

    size_t blockSize() const;
    size_t numBlockGroups() const;
    size_t sectorsPerBlock() const;

    // Medium-level interface
    bool readSuperBlock();
    bool readBlockGroupDescriptorTable();
    estd::unique_ptr<ext2::Inode> readInode(uint32_t ino);

    // Low-level interface
    bool readBlock(void* dest, uint32_t blockId);
    bool readBlock(void* dest, uint32_t blockId, uint32_t maxBytes);
    bool readRange(void* dest, uint32_t blockId, uint32_t numBytes, uint32_t offset = 0);

    DiskDevice& _disk;
    estd::unique_ptr<ext2::SuperBlock> _superBlock;
    estd::unique_ptr<ext2::BlockGroupDescriptor[]> _blockGroups;
    estd::unique_ptr<ext2::Inode> _rootInode;
    Buffer _rootDir;
};
