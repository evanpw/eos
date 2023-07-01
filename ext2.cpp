#include "ext2.h"

#include <string.h>

#include "ide.h"
#include "klibc.h"
#include "panic.h"
#include "units.h"

// s_magic
static constexpr uint16_t EXT2_SUPER_MAGIC = 0xEF53;

enum SState : uint16_t {
    EXT2_VALID_FS = 1,
    EXT2_ERROR_FS = 2,
};

enum SRevLevel : uint32_t {
    EXT2_GOOD_OLD_REV = 0,
    EXT2_DYNAMIC_REV = 1,
};

enum SErrors : uint16_t {
    EXT2_ERRORS_CONTINUE = 1,
    EXT2_ERRORS_RO = 2,
    EXT2_ERRORS_PANIC = 3,
};

enum SCreatorOS : uint32_t {
    EXT2_OS_LINUX = 0,
    EXT2_OS_HURD = 1,
    EXT2_OS_MASIX = 2,
    EXT2_OS_FREEBSD = 3,
    EXT2_OS_LITES = 4,
};

enum SFeatureCompat : uint32_t {
    EXT2_FEATURE_COMPAT_DIR_PREALLOC = 0x0001,
    EXT2_FEATURE_COMPAT_IMAGIC_INODES = 0x0002,
    EXT2_FEATURE_COMPAT_HAS_JOURNAL = 0x0004,
    EXT2_FEATURE_COMPAT_EXT_ATTR = 0x0008,
    EXT2_FEATURE_COMPAT_RESIZE_INO = 0x0010,
    EXT2_FEATURE_COMPAT_DIR_INDEX = 0x0020,
};

enum SFeatureIncompat : uint32_t {
    EXT2_FEATURE_INCOMPAT_COMPRESSION = 0x0001,
    EXT2_FEATURE_INCOMPAT_FILETYPE = 0x0002,
    EXT2_FEATURE_INCOMPAT_RECOVER = 0x0004,
    EXT2_FEATURE_INCOMPAT_JOURNAL_DEV = 0x0008,
    EXT2_FEATURE_INCOMPAT_META_BG = 0x0010,
};

enum SFeatureROCompat : uint32_t {
    EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER = 0x0001,
    EXT2_FEATURE_RO_COMPAT_LARGE_FILE = 0x0002,
    EXT2_FEATURE_RO_COMPAT_BTREE_DIR = 0x0004,
};

enum ReservedInodes : uint32_t {
    EXT2_BAD_INO = 1,
    EXT2_ROOT_INO = 2,
    EXT2_ACL_IDX_INO = 3,
    EXT2_ACL_DATA_INO = 4,
    EXT2_BOOT_LOADER_INO = 5,
    EXT2_UNDEL_DIR_INO = 6,
};

struct __attribute__((packed)) SuperBlock {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;  // not used
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;  // not used
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    SState s_state;
    SErrors s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    SCreatorOS s_creator_os;
    SRevLevel s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    SFeatureCompat s_feature_compat;
    SFeatureIncompat s_feature_incompat;
    SFeatureROCompat s_feature_ro_compat;
    uint8_t s_uuid[16];
    char s_volume_name[16];
    char s_last_mounted[64];
    uint32_t s_algo_bitmap;
    uint8_t s_prealloc_blocks;
    uint8_t s_prealloc_dir_blocks;
    uint16_t _alignment;
    uint8_t s_journal_uuid[16];
    uint32_t s_journal_inum;
    uint32_t s_journal_dev;
    uint32_t s_last_orphan;
    uint32_t s_hash_seed[4];
    uint8_t s_def_hash_version;
    uint8_t _padding[3];
    uint32_t s_default_mount_options;
    uint32_t s_first_meta_bg;
    uint8_t unused[760];
};

static_assert(sizeof(SuperBlock) == 1024);

struct __attribute__((packed)) BlockGroupDescriptor {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    char reserved[12];
};

static_assert(sizeof(BlockGroupDescriptor) == 32);

enum IMode : uint16_t {
    // File format
    EXT2_S_IFSOCK = 0xC000,
    EXT2_S_IFLNK = 0xA000,
    EXT2_S_IFREG = 0x8000,
    EXT2_S_IFBLK = 0x6000,
    EXT2_S_IFDIR = 0x4000,
    EXT2_S_IFCHR = 0x2000,
    EXT2_S_IFIFO = 0x1000,
    // User / group override
    EXT2_S_ISUID = 0x0800,
    EXT2_S_ISGID = 0x0400,
    EXT2_S_ISVTX = 0x0200,
    // Access control
    EXT2_S_IRUSR = 0x0100,
    EXT2_S_IWUSR = 0x0080,
    EXT2_S_IXUSR = 0x0040,
    EXT2_S_IRGRP = 0x0020,
    EXT2_S_IWGRP = 0x0010,
    EXT2_S_IXGRP = 0x0008,
    EXT2_S_IROTH = 0x0004,
    EXT2_S_IWOTH = 0x0002,
    EXT2_S_IXOTH = 0x0001,
};

struct __attribute__((packed)) Inode {
    IMode i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15];
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    char i_osd2[12];

    uint64_t size() {
        if ((i_mode & 0xF000) != EXT2_S_IFREG) {
            return (static_cast<uint64_t>(i_dir_acl) << 32) | i_size;
        } else {
            return i_size;
        }
    }
};

static_assert(sizeof(Inode) == 128);

struct __attribute__((packed)) DirectoryEntry {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[];
};

static_assert(sizeof(DirectoryEntry) == 8);

size_t Ext2Filesystem::blockSize() const {
    return 1024UL << _superBlock->s_log_block_size;
}
size_t Ext2Filesystem::numBlockGroups() const {
    return ceilDiv(_superBlock->s_blocks_count,
                   _superBlock->s_blocks_per_group);
}
size_t Ext2Filesystem::sectorsPerBlock() const {
    return blockSize() / SECTOR_SIZE;
}

bool Ext2Filesystem::readSuperBlock() {
    // The ext2 superblock is always 1024 bytes (2 sectors) at LBA 2 (offset
    // 1024)
    _superBlock = new SuperBlock;
    if (!_disk->readSectors(_superBlock, 2, 2)) {
        return false;
    }

    if (_superBlock->s_magic != EXT2_SUPER_MAGIC) {
        println("ext2: superblock magic number is wrong: {:04X}",
                _superBlock->s_magic);
        return false;
    }

    if (_superBlock->s_rev_level != EXT2_DYNAMIC_REV) {
        println("ext2: unsupported major revision level: {}",
                _superBlock->s_rev_level);
        return false;
    }

    if (_superBlock->s_state != EXT2_VALID_FS) {
        println("ext2: filesystem was not unmounted safely");
        return false;
    }

    if (_superBlock->s_creator_os != EXT2_OS_LINUX) {
        println("ext2: unsupported creator os: {}", _superBlock->s_creator_os);
        return false;
    }

    if (_superBlock->s_first_ino <= 2) {
        println("ext2: no reserved inode for root directory");
        return false;
    }

    if (_superBlock->s_feature_incompat & ~EXT2_FEATURE_INCOMPAT_FILETYPE) {
        println("ext2: unsupported incompatible features");
        return false;
    }

    return true;
}

bool Ext2Filesystem::readBlockGroupDescriptorTable() {
    // Starts on the first block following the superblock
    size_t blockId = _superBlock->s_log_block_size == 0 ? 2 : 1;

    _blockGroups = new BlockGroupDescriptor[numBlockGroups()];

    size_t numBytes = numBlockGroups() * sizeof(BlockGroupDescriptor);
    if (!readRange(_blockGroups, blockId, numBytes)) {
        return false;
    }

    return true;
}

Inode* Ext2Filesystem::readInode(uint32_t ino) {
    uint32_t blockGroup = (ino - 1) / _superBlock->s_inodes_per_group;
    uint32_t index = (ino - 1) % _superBlock->s_inodes_per_group;

    uint32_t blockId = _blockGroups[blockGroup].bg_inode_table;
    uint32_t offset = index * _superBlock->s_inode_size;
    uint32_t size = sizeof(Inode);

    Inode* inode = new Inode;
    if (!readRange(inode, blockId, size, offset)) {
        delete inode;
        return nullptr;
    }

    return inode;
}

uint8_t* Ext2Filesystem::readFile(Inode* inode) {
    size_t numBlocks = ceilDiv(inode->size(), blockSize());
    uint8_t* buffer = new uint8_t[numBlocks * blockSize()];

    size_t blocksRemaining = numBlocks;
    uint8_t* dest = buffer;

    // Direct blocks
    for (size_t i = 0; i < 12 && blocksRemaining > 0; ++i) {
        if (!readBlock(dest, inode->i_block[i])) {
            delete[] buffer;
            return nullptr;
        }

        dest += blockSize();
        --blocksRemaining;
    }

    if (blocksRemaining == 0) {
        return buffer;
    }

    // Indirect blocks
    size_t entriesPerBlock = blockSize() / sizeof(uint32_t);
    uint32_t* indBlock = new uint32_t[entriesPerBlock];
    if (!readBlock(indBlock, inode->i_block[12])) {
        delete[] buffer;
        delete[] indBlock;
        return nullptr;
    }

    for (size_t i = 0; i < entriesPerBlock && blocksRemaining > 0; ++i) {
        if (!readBlock(dest, indBlock[i])) {
            delete[] buffer;
            delete[] indBlock;
            return nullptr;
        }

        dest += blockSize();
        --blocksRemaining;
    }

    // TODO: support doubly-indirect and triply-indirect blocks
    ASSERT(blocksRemaining == 0);
    return buffer;
}

bool Ext2Filesystem::readBlock(void* dest, uint32_t blockId) {
    size_t lba = blockId * sectorsPerBlock();
    size_t numSectors = sectorsPerBlock();

    if (!_disk->readSectors(dest, lba, numSectors)) {
        return false;
    }

    return true;
}

bool Ext2Filesystem::readRange(void* dest, uint32_t blockId, uint32_t numBytes,
                               uint32_t offset) {
    size_t lba = blockId * sectorsPerBlock();
    size_t numSectors = ceilDiv(offset + numBytes, SECTOR_SIZE);

    if (offset >= SECTOR_SIZE) {
        size_t skipSectors = offset / SECTOR_SIZE;
        lba += skipSectors;
        numSectors -= skipSectors;
        offset -= skipSectors * SECTOR_SIZE;
    }

    uint8_t* buffer = new uint8_t[SECTOR_SIZE * numSectors];
    if (!_disk->readSectors(buffer, lba, numSectors)) {
        delete[] buffer;
        return false;
    }

    memcpy(dest, buffer + offset, numBytes);
    delete[] buffer;
    return true;
}

bool Ext2Filesystem::init(IDEDevice* disk) {
    ASSERT(disk);
    _disk = disk;

    if (!readSuperBlock()) {
        println("ext2: error while reading superblock");
        return false;
    }

    if (!readBlockGroupDescriptorTable()) {
        println("ext2: error while reading block group descriptor table");
        return false;
    }

    Inode* rootInode = readInode(EXT2_ROOT_INO);
    if (!rootInode) {
        println("ext2: error while reading root directory inode");
        return false;
    }

    if ((rootInode->i_mode & 0xF000) != EXT2_S_IFDIR) {
        println("ext2: root directory is not a directory");
        delete rootInode;
        return false;
    }

    uint8_t* rootDir = readFile(rootInode);
    if (!rootDir) {
        println("ext2: failed to read root directory");
        delete rootInode;
        return false;
    }

    char nameBuffer[256];

    uint32_t ino = 0;
    uint16_t offset = 0;
    while (offset < rootInode->size()) {
        DirectoryEntry dirEntry;
        memcpy(&dirEntry, rootDir + offset, sizeof(dirEntry));

        memcpy(nameBuffer, rootDir + offset + sizeof(dirEntry),
               dirEntry.name_len);
        nameBuffer[dirEntry.name_len] = '\0';

        if (strncmp(nameBuffer, "dictionary.txt", dirEntry.name_len + 1) == 0) {
            ino = dirEntry.inode;
        }

        offset += dirEntry.rec_len;
    }

    if (ino == 0) {
        println("ext2: file not found: 'dictionary.txt'");
        delete[] rootDir;
        delete rootInode;
        return false;
    }

    Inode* inode = readInode(ino);
    if (!inode) {
        println("ext2: can't read inode for file 'dictionary.txt'");
        delete[] rootDir;
        delete rootInode;
        return false;
    }

    uint8_t* fileData = readFile(inode);
    if (!fileData) {
        delete inode;
        delete[] rootDir;
        delete rootInode;
        println("ext2: can't read file 'dictionary.txt'");
        return false;
    }

    delete fileData;
    delete inode;
    delete[] rootDir;
    delete rootInode;

    println("ext2 filesystem initialized");
    return true;
}

Ext2Filesystem::~Ext2Filesystem() {
    delete _superBlock;
    delete _blockGroups;
}
