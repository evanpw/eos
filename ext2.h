#pragma once
#include "ide.h"

struct SuperBlock;
struct BlockGroupDescriptor;
struct Inode;

class Ext2Filesystem {
public:
    // Create using a static method because creation can fail
    static Ext2Filesystem* create(IDEDevice* disk) {
        Ext2Filesystem* fs = new Ext2Filesystem;

        if (!fs->init(disk)) {
            delete fs;
            return nullptr;
        }

        return fs;
    }

    ~Ext2Filesystem();

private:
    Ext2Filesystem() = default;
    bool init(IDEDevice* disk);

    // Medium-level interface
    bool readSuperBlock();
    bool readBlockGroupDescriptorTable();
    Inode* readInode(uint32_t ino);
    uint8_t* readFile(Inode* inode);

    // Low-level interface
    bool readBlock(void* dest, uint32_t blockId);
    bool readRange(void* dest, uint32_t blockId, uint32_t numBytes,
                   uint32_t offset = 0);

    IDEDevice* _disk = 0;
    SuperBlock* _superBlock = 0;
    BlockGroupDescriptor* _blockGroups = 0;
};
