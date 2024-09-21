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
    uint32_t lookup(uint32_t cwdIno, const char* path);
    uint32_t getParent(const ext2::Inode& inode);
    int getPath(uint32_t ino, char* path, size_t pathSize);
    bool readFullFile(const ext2::Inode& inode, uint8_t* dest);
    ssize_t readFromFile(const ext2::Inode& inode, uint8_t* dest, uint32_t size,
                         uint32_t offset = 0);

    // Medium-level interface
    estd::unique_ptr<ext2::Inode> readInode(uint32_t ino);

private:
    Ext2FileSystem(DiskDevice& disk);
    bool init();

    size_t blockSize() const;
    size_t numBlockGroups() const;
    size_t sectorsPerBlock() const;

    // Low-level interface
    bool readSuperBlock();
    bool readBlockGroupDescriptorTable();
    bool readBlock(void* dest, uint32_t blockId);
    bool readBlock(void* dest, uint32_t blockId, uint32_t maxBytes);
    bool readRange(void* dest, uint32_t blockId, uint32_t numBytes, uint32_t offset = 0);

    DiskDevice& _disk;
    estd::unique_ptr<ext2::SuperBlock> _superBlock;
    estd::unique_ptr<ext2::BlockGroupDescriptor[]> _blockGroups;
    estd::unique_ptr<ext2::Inode> _rootInode;
    Buffer _rootDir;
};
