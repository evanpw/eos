#include "fs/ext2.h"

#include <string.h>

#include "ide.h"
#include "klibc.h"
#include "panic.h"
#include "units.h"

size_t Ext2FileSystem::blockSize() const {
    return 1024UL << _superBlock->s_log_block_size;
}
size_t Ext2FileSystem::numBlockGroups() const {
    return ceilDiv(_superBlock->s_blocks_count,
                   _superBlock->s_blocks_per_group);
}
size_t Ext2FileSystem::sectorsPerBlock() const {
    return blockSize() / SECTOR_SIZE;
}

bool Ext2FileSystem::readSuperBlock() {
    // The ext2 superblock is always 1024 bytes (2 sectors) at LBA 2 (offset
    // 1024)
    _superBlock.assign(new ext2::SuperBlock);
    if (!_disk.readSectors(_superBlock.get(), 2, 2)) {
        return false;
    }

    if (_superBlock->s_magic != ext2::EXT2_SUPER_MAGIC) {
        println("ext2: superblock magic number is wrong: {:04X}",
                _superBlock->s_magic);
        return false;
    }

    if (_superBlock->s_rev_level != ext2::EXT2_DYNAMIC_REV) {
        println("ext2: unsupported major revision level: {}",
                _superBlock->s_rev_level);
        return false;
    }

    if (_superBlock->s_state != ext2::EXT2_VALID_FS) {
        println("ext2: filesystem was not unmounted safely");
        return false;
    }

    if (_superBlock->s_creator_os != ext2::EXT2_OS_LINUX) {
        println("ext2: unsupported creator os: {}", _superBlock->s_creator_os);
        return false;
    }

    if (_superBlock->s_first_ino <= 2) {
        println("ext2: no reserved inode for root directory");
        return false;
    }

    if (_superBlock->s_feature_incompat &
        ~ext2::EXT2_FEATURE_INCOMPAT_FILETYPE) {
        println("ext2: unsupported incompatible features");
        return false;
    }

    return true;
}

bool Ext2FileSystem::readBlockGroupDescriptorTable() {
    // Starts on the first block following the superblock
    size_t blockId = _superBlock->s_log_block_size == 0 ? 2 : 1;

    _blockGroups.assign(new ext2::BlockGroupDescriptor[numBlockGroups()]);

    size_t numBytes = numBlockGroups() * sizeof(ext2::BlockGroupDescriptor);
    if (!readRange(_blockGroups.get(), blockId, numBytes)) {
        return false;
    }

    return true;
}

OwnPtr<ext2::Inode> Ext2FileSystem::readInode(uint32_t ino) {
    uint32_t blockGroup = (ino - 1) / _superBlock->s_inodes_per_group;
    uint32_t index = (ino - 1) % _superBlock->s_inodes_per_group;

    uint32_t blockId = _blockGroups[blockGroup].bg_inode_table;
    uint32_t offset = index * _superBlock->s_inode_size;

    OwnPtr<ext2::Inode> inode(new ext2::Inode);
    if (!readRange(inode.get(), blockId, sizeof(ext2::Inode), offset)) {
        return {};
    }

    return inode;
}

bool Ext2FileSystem::readFile(uint8_t* dest, const ext2::Inode& inode) {
    size_t numBlocks = ceilDiv(inode.size(), blockSize());

    size_t blocksRemaining = numBlocks;
    size_t bytesRemaining = inode.size();

    // Direct blocks
    for (size_t i = 0; i < 12; ++i) {
        ASSERT((bytesRemaining <= blockSize()) == (blocksRemaining == 1));

        if (!readBlock(dest, inode.i_block[i], bytesRemaining)) {
            return false;
        }

        --blocksRemaining;

        if (blocksRemaining == 0) {
            return true;
        }

        dest += blockSize();
        bytesRemaining -= blockSize();
    }

    // Indirect blocks
    size_t entriesPerBlock = blockSize() / sizeof(uint32_t);
    OwnPtr<uint32_t[]> indBlock(new uint32_t[entriesPerBlock]);
    if (!readBlock(indBlock.get(), inode.i_block[12])) {
        return false;
    }

    for (size_t i = 0; i < entriesPerBlock; ++i) {
        ASSERT((bytesRemaining <= blockSize()) == (blocksRemaining == 1));

        if (!readBlock(dest, indBlock[i], bytesRemaining)) {
            return false;
        }

        --blocksRemaining;

        if (blocksRemaining == 0) {
            return true;
        }

        dest += blockSize();
        bytesRemaining -= blockSize();
    }

    // TODO: support doubly-indirect and triply-indirect blocks
    println("ext2: doubly- and triply-indirect blocks are unsupported");
    return false;
}

bool Ext2FileSystem::readBlock(void* dest, uint32_t blockId) {
    size_t lba = blockId * sectorsPerBlock();
    size_t numSectors = sectorsPerBlock();

    if (!_disk.readSectors(dest, lba, numSectors)) {
        return false;
    }

    return true;
}

bool Ext2FileSystem::readBlock(void* dest, uint32_t blockId,
                               uint32_t maxBytes) {
    if (maxBytes >= blockSize()) {
        return readBlock(dest, blockId);
    }

    Buffer buffer(blockSize());
    if (!readBlock(buffer.get(), blockId)) {
        return false;
    }

    memcpy(dest, buffer.get(), maxBytes);
    return true;
}

bool Ext2FileSystem::readRange(void* dest, uint32_t blockId, uint32_t numBytes,
                               uint32_t offset) {
    size_t lba = blockId * sectorsPerBlock();
    size_t numSectors = ceilDiv(offset + numBytes, SECTOR_SIZE);

    if (offset >= SECTOR_SIZE) {
        size_t skipSectors = offset / SECTOR_SIZE;
        lba += skipSectors;
        numSectors -= skipSectors;
        offset -= skipSectors * SECTOR_SIZE;
    }

    Buffer buffer(numSectors * SECTOR_SIZE);
    if (!_disk.readSectors(buffer.get(), lba, numSectors)) {
        return false;
    }

    memcpy(dest, &buffer[offset], numBytes);
    return true;
}

bool Ext2FileSystem::init() {
    if (!readSuperBlock()) {
        println("ext2: error while reading superblock");
        return false;
    }

    if (!readBlockGroupDescriptorTable()) {
        println("ext2: error while reading block group descriptor table");
        return false;
    }

    _rootInode = readInode(ext2::EXT2_ROOT_INO);
    if (!_rootInode) {
        println("ext2: error while reading root directory inode");
        return false;
    }

    if ((_rootInode->i_mode & 0xF000) != ext2::EXT2_S_IFDIR) {
        println("ext2: root directory is not a directory");
        return false;
    }

    _rootDir = Buffer(_rootInode->size());
    if (!readFile(_rootDir.get(), *_rootInode)) {
        println("ext2: failed to read root directory");
        return false;
    }

    println("ext2 filesystem initialized");
    return true;
}

OwnPtr<ext2::Inode> Ext2FileSystem::lookup(const char* path) {
    char nameBuffer[256];

    uint32_t ino = 0;
    uint16_t offset = 0;
    while (offset < _rootInode->size()) {
        ext2::DirectoryEntry dirEntry;
        memcpy(&dirEntry, &_rootDir[offset], sizeof(dirEntry));

        memcpy(nameBuffer, &_rootDir[offset + sizeof(dirEntry)],
               dirEntry.name_len);
        nameBuffer[dirEntry.name_len] = '\0';

        if (strncmp(nameBuffer, path, dirEntry.name_len + 1) == 0) {
            ino = dirEntry.inode;
        }

        offset += dirEntry.rec_len;
    }

    if (ino == 0) {
        println("ext2: file not found: '{}'", path);
        return {};
    }

    return readInode(ino);
}

OwnPtr<Ext2FileSystem> Ext2FileSystem::create(IDEDevice& disk) {
    OwnPtr<Ext2FileSystem> fs(new Ext2FileSystem(disk));

    if (!fs->init()) {
        return {};
    }

    return fs;
}

Ext2FileSystem::Ext2FileSystem(IDEDevice& disk) : _disk(disk) {}
Ext2FileSystem::~Ext2FileSystem() = default;
