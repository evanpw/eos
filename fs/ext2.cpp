#include "fs/ext2.h"

#include <string.h>

#include "api/errno.h"
#include "estd/print.h"
#include "klibc.h"
#include "units.h"

size_t Ext2FileSystem::blockSize() const { return 1024UL << _superBlock->log_block_size; }
size_t Ext2FileSystem::numBlockGroups() const {
    return ceilDiv(_superBlock->blocks_count, _superBlock->blocks_per_group);
}
size_t Ext2FileSystem::sectorsPerBlock() const { return blockSize() / SECTOR_SIZE; }

bool Ext2FileSystem::readSuperBlock() {
    // The ext2 superblock is always 1024 bytes (2 sectors) at LBA 2 (offset
    // 1024)
    _superBlock.assign(new ext2::SuperBlock);
    if (!_disk.readSectors(_superBlock.get(), 2, 2)) {
        return false;
    }

    if (_superBlock->magic != ext2::SUPER_MAGIC) {
        println("ext2: superblock magic number is wrong: {:04X}", _superBlock->magic);
        return false;
    }

    if (_superBlock->rev_level != ext2::DYNAMIC_REV) {
        println("ext2: unsupported major revision level: {}", _superBlock->rev_level);
        return false;
    }

    if (_superBlock->state != ext2::VALID_FS) {
        println("ext2: filesystem was not unmounted safely");
        return false;
    }

    if (_superBlock->creator_os != ext2::OS_LINUX) {
        println("ext2: unsupported creator os: {}", _superBlock->creator_os);
        return false;
    }

    if (_superBlock->first_ino <= 2) {
        println("ext2: no reserved inode for root directory");
        return false;
    }

    if (_superBlock->feature_incompat & ~ext2::FEATURE_INCOMPAT_FILETYPE) {
        println("ext2: unsupported incompatible features");
        return false;
    }

    return true;
}

bool Ext2FileSystem::readBlockGroupDescriptorTable() {
    // Starts on the first block following the superblock
    size_t blockId = _superBlock->log_block_size == 0 ? 2 : 1;

    _blockGroups.assign(new ext2::BlockGroupDescriptor[numBlockGroups()]);

    size_t numBytes = numBlockGroups() * sizeof(ext2::BlockGroupDescriptor);
    if (!readRange(_blockGroups.get(), blockId, numBytes)) {
        return false;
    }

    return true;
}

estd::unique_ptr<ext2::Inode> Ext2FileSystem::readInode(uint32_t ino) {
    uint32_t blockGroup = (ino - 1) / _superBlock->inodes_per_group;
    uint32_t index = (ino - 1) % _superBlock->inodes_per_group;

    uint32_t blockId = _blockGroups[blockGroup].inode_table;
    uint32_t offset = index * _superBlock->inode_size;

    estd::unique_ptr<ext2::Inode> inode(new ext2::Inode);
    if (!readRange(inode.get(), blockId, sizeof(ext2::Inode), offset)) {
        return {};
    }

    return inode;
}

bool Ext2FileSystem::readFullFile(const ext2::Inode& inode, uint8_t* dest) {
    size_t numBlocks = ceilDiv(inode.size(), blockSize());

    size_t blocksRemaining = numBlocks;
    size_t bytesRemaining = inode.size();

    // Direct blocks
    for (size_t i = 0; i < 12; ++i) {
        ASSERT((bytesRemaining <= blockSize()) == (blocksRemaining == 1));

        if (!readBlock(dest, inode.block[i], bytesRemaining)) {
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
    estd::unique_ptr<uint32_t[]> indBlock(new uint32_t[entriesPerBlock]);
    if (!readBlock(indBlock.get(), inode.block[12])) {
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

ssize_t Ext2FileSystem::readFromFile(const ext2::Inode& inode, uint8_t* dest,
                                     uint32_t size, uint32_t offset) {
    // Clip the read at the end of the file
    if (offset > inode.size()) {
        return 0;
    } else if (size > inode.size() - offset) {
        size = inode.size() - offset;
    }

    // TODO: this is insanely inefficient, should read only what's needed
    size_t numBlocks = ceilDiv(inode.size(), blockSize());

    Buffer buffer(numBlocks * blockSize());
    if (!readFullFile(inode, buffer.get())) return -EIO;

    memcpy(dest, &buffer[offset], size);
    return size;
}

bool Ext2FileSystem::readBlock(void* dest, uint32_t blockId) {
    size_t lba = blockId * sectorsPerBlock();
    size_t numSectors = sectorsPerBlock();

    if (!_disk.readSectors(dest, lba, numSectors)) {
        return false;
    }

    return true;
}

bool Ext2FileSystem::readBlock(void* dest, uint32_t blockId, uint32_t maxBytes) {
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

    _rootInode = readInode(ext2::ROOT_INO);
    if (!_rootInode) {
        println("ext2: error while reading root directory inode");
        return false;
    }

    if (!_rootInode->isDirectory()) {
        println("ext2: root directory is not a directory");
        return false;
    }

    _rootDir = Buffer(_rootInode->size());
    ASSERT(_rootInode->size() % blockSize() == 0);
    if (!readFullFile(*_rootInode, _rootDir.get())) {
        println("ext2: failed to read root directory");
        return false;
    }

    println("ext2: init complete");
    return true;
}

uint32_t Ext2FileSystem::lookup(uint32_t cwdIno, const char* path) {
    // TODO: check path pointer points to valid memory

    // If the path is absolute, start at the root directory rather than the cwd
    uint32_t currentIno;
    if (path[0] == '/') {
        currentIno = ext2::ROOT_INO;
    } else {
        currentIno = cwdIno;
    }

    estd::unique_ptr<ext2::Inode> current = readInode(currentIno);

    const char* p = path;
    while (true) {
        while (*p == '/') {
            ++p;
        }

        // If we've reached the end of the path, we're done
        if (*p == '\0') {
            return currentIno;
        }

        // Find the next component of the path
        const char* next = strchr(p, '/');
        if (!next) {
            next = p + strlen(p);
        }

        // Make sure that current location is a directory
        if (!current->isDirectory()) {
            return ext2::BAD_INO;
        }

        Buffer dirBuffer(current->size());
        if (!readFullFile(*current, dirBuffer.get())) {
            println("ext2: corrupt filesystem when looking up '{}'", path);
            return ext2::BAD_INO;
        }

        // Search for the next component in the current directory
        ext2::DirectoryEntry* foundEntry = nullptr;
        uint64_t offset = 0;
        while (offset < current->size()) {
            ext2::DirectoryEntry* dirEntry =
                reinterpret_cast<ext2::DirectoryEntry*>(&dirBuffer[offset]);

            if (dirEntry->name_len == next - p &&
                strncmp(p, dirEntry->name, next - p) == 0) {
                foundEntry = dirEntry;
                break;
            }

            offset += dirEntry->rec_len;
        }

        if (!foundEntry) {
            return ext2::BAD_INO;
        }

        // Load the inode for the next component
        currentIno = foundEntry->inode;
        current = readInode(currentIno);
        if (!current) {
            println("ext2: corrupt filesystem when looking up '{}'", path);
            return ext2::BAD_INO;
        }

        p = next;
    }
}

static int prependString(char* dest, const char* src, size_t srcLength, size_t n) {
    size_t destLen = strlen(dest);
    if (srcLength + destLen + 1 > n) {
        return -ENAMETOOLONG;
    }

    memmove(dest + srcLength, dest, destLen + 1);
    memcpy(dest, src, srcLength);
    return 0;
}

uint32_t Ext2FileSystem::getParent(const ext2::Inode& inode) {
    ASSERT(inode.isDirectory());

    // Read the contents of the current directory
    Buffer dirBuffer(inode.size());
    if (!readFullFile(inode, dirBuffer.get())) {
        return -1;
    }

    // Search for the parent directory ".."
    uint64_t offset = 0;
    while (offset < inode.size()) {
        ext2::DirectoryEntry* dirEntry =
            reinterpret_cast<ext2::DirectoryEntry*>(&dirBuffer[offset]);

        if (dirEntry->name_len == 2 &&
            strncmp(dirEntry->name, "..", dirEntry->name_len) == 0) {
            return dirEntry->inode;
        }

        offset += dirEntry->rec_len;
    }

    return ext2::BAD_INO;
}

int Ext2FileSystem::getPath(uint32_t ino, char* path, size_t pathSize) {
    if (pathSize == 0) {
        return -EINVAL;
    }

    path[0] = '\0';

    if (ino == ext2::ROOT_INO) {
        return prependString(path, "/", 1, pathSize);
    }

    estd::unique_ptr<ext2::Inode> inode = readInode(ino);
    if (!inode) {
        return -EIO;
    }

    uint32_t currentIno = ino;
    while (true) {
        if (!inode->isDirectory()) {
            return -ENOTDIR;
        }

        uint32_t parentIno = getParent(*inode);
        if (parentIno == ext2::BAD_INO) {
            return -EIO;
        }

        estd::unique_ptr<ext2::Inode> parent = readInode(parentIno);
        if (!parent) {
            return -EIO;
        }

        // Search the parent directory for an entry pointing to currentIno
        Buffer dirBuffer(parent->size());
        if (!readFullFile(*parent, dirBuffer.get())) {
            return -EIO;
        }

        ext2::DirectoryEntry* foundEntry = nullptr;
        uint64_t offset = 0;
        while (offset < parent->size()) {
            ext2::DirectoryEntry* dirEntry =
                reinterpret_cast<ext2::DirectoryEntry*>(&dirBuffer[offset]);

            if (dirEntry->inode == currentIno) {
                foundEntry = dirEntry;
                break;
            }

            offset += dirEntry->rec_len;
        }

        if (!foundEntry) {
            return -ENOENT;
        }

        // Add the name of the current directory to the beginning of the path, preceded by
        // a forward slash
        if (prependString(path, foundEntry->name, foundEntry->name_len, pathSize) < 0) {
            return -ENAMETOOLONG;
        }

        if (prependString(path, "/", 1, pathSize) < 0) {
            return -ENAMETOOLONG;
        }

        // Terminate once we reach the root directory
        if (parentIno == ext2::ROOT_INO) {
            return 0;
        }

        currentIno = parentIno;
        inode = estd::move(parent);
    }
}

estd::unique_ptr<Ext2FileSystem> Ext2FileSystem::create(DiskDevice& disk) {
    estd::unique_ptr<Ext2FileSystem> fs(new Ext2FileSystem(disk));

    if (!fs->init()) {
        return {};
    }

    return fs;
}

Ext2FileSystem::Ext2FileSystem(DiskDevice& disk) : _disk(disk) {}
Ext2FileSystem::~Ext2FileSystem() = default;
