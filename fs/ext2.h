#pragma once
#include "disk.h"
#include "estd/buffer.h"
#include "estd/ownptr.h"
#include "fs/ext2_defs.h"

class Ext2FileSystem {
public:
    // Create using a static method because creation can fail
    static OwnPtr<Ext2FileSystem> create(DiskDevice& disk);
    ~Ext2FileSystem();

    // High-level interface
    OwnPtr<ext2::Inode> lookup(const char* path);
    bool readFile(uint8_t* dest, const ext2::Inode& inode);

private:
    Ext2FileSystem(DiskDevice& disk);
    bool init();

    size_t blockSize() const;
    size_t numBlockGroups() const;
    size_t sectorsPerBlock() const;

    // Medium-level interface
    bool readSuperBlock();
    bool readBlockGroupDescriptorTable();
    OwnPtr<ext2::Inode> readInode(uint32_t ino);

    // Low-level interface
    bool readBlock(void* dest, uint32_t blockId);
    bool readBlock(void* dest, uint32_t blockId, uint32_t maxBytes);
    bool readRange(void* dest, uint32_t blockId, uint32_t numBytes,
                   uint32_t offset = 0);

    DiskDevice& _disk;
    OwnPtr<ext2::SuperBlock> _superBlock;
    OwnPtr<ext2::BlockGroupDescriptor[]> _blockGroups;
    OwnPtr<ext2::Inode> _rootInode;
    Buffer _rootDir;
};
