#pragma once
#include "estd/ownptr.h"
#include "ide.h"

struct SuperBlock;
struct BlockGroupDescriptor;
struct Inode;

class Ext2Filesystem {
public:
    // Create using a static method because creation can fail
    static OwnPtr<Ext2Filesystem> create(IDEDevice& disk);
    ~Ext2Filesystem();

private:
    Ext2Filesystem(IDEDevice& disk);
    bool init();

    size_t blockSize() const;
    size_t numBlockGroups() const;
    size_t sectorsPerBlock() const;

    // Medium-level interface
    bool readSuperBlock();
    bool readBlockGroupDescriptorTable();
    OwnPtr<Inode> readInode(uint32_t ino);
    uint8_t* readFile(Inode& inode);

    // Low-level interface
    bool readBlock(void* dest, uint32_t blockId);
    bool readRange(void* dest, uint32_t blockId, uint32_t numBytes,
                   uint32_t offset = 0);

    IDEDevice& _disk;
    OwnPtr<SuperBlock> _superBlock;
    OwnPtr<BlockGroupDescriptor[]> _blockGroups;
};
