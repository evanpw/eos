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

enum ReservedINodes : uint32_t {
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

    size_t blockSize() const { return 1024UL << s_log_block_size; }
    size_t numBlockGroups() const {
        return ceilDiv(s_blocks_count, s_blocks_per_group);
    }
    size_t sectorsPerBlock() const { return blockSize() / SECTOR_SIZE; }
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

struct __attribute__((packed)) INode {
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

static_assert(sizeof(INode) == 128);

struct __attribute__((packed)) DirectoryEntry {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[];
};

static_assert(sizeof(DirectoryEntry) == 8);

SuperBlock* readSuperBlock() {
    // The ext2 superblock is always 1024 bytes (2 sectors) at LBA 2 (offset
    // 1024)
    SuperBlock* superBlock = new SuperBlock;
    if (!g_hardDrive2->readSectors(superBlock, 2, 2)) {
        panic("ext2: failed to read superblock");
    }

    if (superBlock->s_magic != EXT2_SUPER_MAGIC) {
        println("ext2: superblock magic number is wrong: {:04X}",
                superBlock->s_magic);
        panic();
    }

    if (superBlock->s_rev_level != EXT2_DYNAMIC_REV) {
        println("ext2: unsupported major revision level: {}",
                superBlock->s_rev_level);
        panic();
    }

    if (superBlock->s_state != EXT2_VALID_FS) {
        panic("ext2: filesystem was not unmounted safely");
    }

    if (superBlock->s_creator_os != EXT2_OS_LINUX) {
        println("ext2: unsupported creator os: {}", superBlock->s_creator_os);
        panic();
    }

    if (superBlock->s_first_ino <= 2) {
        panic("ext2: no reserved inode for root directory");
    }

    println("number of inodes: {}", superBlock->s_inodes_count);
    println("number of blocks: {}", superBlock->s_blocks_count);
    println("number of reserved blocks: {}", superBlock->s_r_blocks_count);
    println("number of free blocks: {}", superBlock->s_free_blocks_count);
    println("number of free inodes: {}", superBlock->s_free_inodes_count);
    println("block size: {} KiB", superBlock->blockSize() / KiB);
    println("blocks per group: {}", superBlock->s_blocks_per_group);
    println("inodes per group: {}", superBlock->s_inodes_per_group);
    println("first inode index: {}", superBlock->s_first_ino);
    println("inode size: {}", superBlock->s_inode_size);

    print("features:");
    if (superBlock->s_feature_compat & EXT2_FEATURE_COMPAT_DIR_PREALLOC) {
        print(" EXT2_FEATURE_COMPAT_DIR_PREALLOC");
    }
    if (superBlock->s_feature_compat & EXT2_FEATURE_COMPAT_IMAGIC_INODES) {
        print(" EXT2_FEATURE_COMPAT_IMAGIC_INODES");
    }
    if (superBlock->s_feature_compat & EXT2_FEATURE_COMPAT_HAS_JOURNAL) {
        print(" EXT2_FEATURE_COMPAT_HAS_JOURNAL");
    }
    if (superBlock->s_feature_compat & EXT2_FEATURE_COMPAT_EXT_ATTR) {
        print(" EXT2_FEATURE_COMPAT_EXT_ATTR");
    }
    if (superBlock->s_feature_compat & EXT2_FEATURE_COMPAT_RESIZE_INO) {
        print(" EXT2_FEATURE_COMPAT_RESIZE_INO");
    }
    if (superBlock->s_feature_compat & EXT2_FEATURE_COMPAT_DIR_INDEX) {
        print(" EXT2_FEATURE_COMPAT_DIR_INDEX");
    }
    if (superBlock->s_feature_incompat & EXT2_FEATURE_INCOMPAT_COMPRESSION) {
        print(" EXT2_FEATURE_INCOMPAT_COMPRESSION");
    }
    if (superBlock->s_feature_incompat & EXT2_FEATURE_INCOMPAT_FILETYPE) {
        print(" EXT2_FEATURE_INCOMPAT_FILETYPE");
    }
    if (superBlock->s_feature_incompat & EXT2_FEATURE_INCOMPAT_RECOVER) {
        print(" EXT2_FEATURE_INCOMPAT_RECOVER");
    }
    if (superBlock->s_feature_incompat & EXT2_FEATURE_INCOMPAT_JOURNAL_DEV) {
        print(" EXT2_FEATURE_JOURNAL_DEV");
    }
    if (superBlock->s_feature_incompat & EXT2_FEATURE_INCOMPAT_META_BG) {
        print(" EXT2_FEATURE_META_BG");
    }
    if (superBlock->s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER) {
        print(" EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER");
    }
    if (superBlock->s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_LARGE_FILE) {
        print(" EXT2_FEATURE_RO_COMPAT_LARGE_FILE");
    }
    if (superBlock->s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_BTREE_DIR) {
        print(" EXT2_FEATURE_RO_COMPAT_BTREE_DIR");
    }
    println("");

    if ((superBlock->s_feature_incompat & ~EXT2_FEATURE_INCOMPAT_FILETYPE) !=
        0) {
        panic("ext2: unsupported incompatible features");
    }

    return superBlock;
}

uint8_t* readBlocks(SuperBlock* superBlock, uint32_t blockId,
                    uint32_t numBlocks) {
    size_t lba = blockId * superBlock->sectorsPerBlock();
    size_t numSectors = numBlocks * superBlock->sectorsPerBlock();

    uint8_t* buffer = new uint8_t[numBlocks * superBlock->blockSize()];
    if (!g_hardDrive2->readSectors(buffer, lba, numSectors)) {
        delete[] buffer;
        return nullptr;
    }

    return buffer;
}

bool readBlock(SuperBlock* superBlock, void* dest, uint32_t blockId) {
    size_t lba = blockId * superBlock->sectorsPerBlock();
    size_t numSectors = superBlock->sectorsPerBlock();

    if (!g_hardDrive2->readSectors(dest, lba, numSectors)) {
        return false;
    }

    return true;
}

bool readFromBlocks(SuperBlock* superBlock, void* dest, uint32_t blockId,
                    uint32_t numBytes, uint32_t offset = 0) {
    size_t lba = blockId * superBlock->sectorsPerBlock();
    size_t numSectors = ceilDiv(offset + numBytes, SECTOR_SIZE);

    if (offset >= SECTOR_SIZE) {
        size_t skipSectors = offset / SECTOR_SIZE;
        lba -= skipSectors;
        numSectors -= skipSectors;
        offset -= skipSectors * SECTOR_SIZE;
    }

    uint8_t* buffer = new uint8_t[SECTOR_SIZE * numSectors];
    if (!g_hardDrive2->readSectors(buffer, lba, numSectors)) {
        delete[] buffer;
        return false;
    }

    memcpy(dest, buffer + offset, numBytes);
    return true;
}

BlockGroupDescriptor* readBlockGroupDescriptorTable(SuperBlock* superBlock) {
    // Starts on the first block following the superblock
    size_t blockId = superBlock->s_log_block_size == 0 ? 2 : 1;

    size_t numBlockGroups = superBlock->numBlockGroups();
    BlockGroupDescriptor* blockGroupDescriptorTable =
        new BlockGroupDescriptor[numBlockGroups];

    size_t numBytes = numBlockGroups * sizeof(BlockGroupDescriptor);
    if (!readFromBlocks(superBlock, blockGroupDescriptorTable, blockId,
                        numBytes)) {
        panic("Failed to read block group descriptor table");
    }

    return blockGroupDescriptorTable;

    /*
    // ??
    ASSERT(superBlock->s_inodes_count == superBlock->s_inodes_per_group *
    numBlockGroups);

    uint32_t blocksRemaining = superBlock->s_blocks_count;
    uint32_t inodesRemaining = superBlock->s_inodes_count;
    for (size_t i = 0; i < numBlockGroups; ++i) {
        println("Block Group {}:", i);
        println("bg_block_bitmap: {}",
    blockGroupDescriptorTable[i].bg_block_bitmap); println("bg_inode_bitmap:
    {}", blockGroupDescriptorTable[i].bg_inode_bitmap); println("bg_inode_table:
    {}", blockGroupDescriptorTable[i].bg_inode_table);
        println("bg_free_blocks_count: {}",
    blockGroupDescriptorTable[i].bg_free_blocks_count);
        println("bg_free_inodes_count: {}",
    blockGroupDescriptorTable[i].bg_free_inodes_count);
        println("bg_used_dirs_count: {}",
    blockGroupDescriptorTable[i].bg_used_dirs_count);

        size_t blocksInGroup = min(superBlock->s_blocks_per_group,
    blocksRemaining); blocksRemaining -= blocksInGroup;

        size_t blockBitmapSize = ceilDiv(blocksInGroup, 8);
        uint8_t* blockBitmap = new uint8_t[blockBitmapSize];
        if (!readFromBlocks(superBlock, blockBitmap,
    blockGroupDescriptorTable[i].bg_block_bitmap, blockBitmapSize)) {
            panic("Failed to read block bitmap");
        }

        size_t inodesInGroup = min(superBlock->s_inodes_per_group,
    inodesRemaining); inodesRemaining -= inodesInGroup;

        size_t inodeBitmapSize = ceilDiv(inodesInGroup, 8);
        uint8_t* inodeBitmap = new uint8_t[inodeBitmapSize];
        if (!readFromBlocks(superBlock, inodeBitmap,
    blockGroupDescriptorTable[i].bg_inode_bitmap, inodeBitmapSize)) {
            panic("Failed to read inode bitmap");
        }

        INode* inodeTable = new INode[superBlock->s_inodes_per_group];
        if (!readFromBlocks(superBlock, inodeTable,
    blockGroupDescriptorTable[i].bg_inode_table, superBlock->s_inodes_per_group
    * sizeof(INode))) { panic("Failed to read inode table");
        }
    }
    */
}

INode* readINode(SuperBlock* superBlock,
                 BlockGroupDescriptor* blockGroupDescriptorTable,
                 uint32_t ino) {
    uint32_t blockGroup = (ino - 1) / superBlock->s_inodes_per_group;
    uint32_t index = (ino - 1) % superBlock->s_inodes_per_group;

    uint32_t blockId = blockGroupDescriptorTable[blockGroup].bg_inode_table;
    uint32_t offset = index * superBlock->s_inode_size;
    uint32_t size = sizeof(INode);

    INode* inode = new INode;
    if (!readFromBlocks(superBlock, inode, blockId, size, offset)) {
        panic("ext2: unable to read root directory inode");
    }

    return inode;
}

uint8_t* readFile(SuperBlock* superBlock, INode* inode) {
    size_t numBlocks = inode->i_blocks * SECTOR_SIZE / superBlock->blockSize();
    ASSERT(numBlocks <=
           12);  // TODO handle indirect and multiply-indirect blocks
    ASSERT(numBlocks * superBlock->blockSize() >= inode->size());
    ASSERT(numBlocks * superBlock->blockSize() - inode->size() <
           superBlock->blockSize());

    uint8_t* buffer = new uint8_t[numBlocks * superBlock->blockSize()];

    uint8_t* dest = buffer;
    for (size_t i = 0; i < numBlocks; ++i) {
        if (!readBlock(superBlock, dest, inode->i_block[i])) {
            delete[] buffer;
            return nullptr;
        }

        dest += superBlock->blockSize();
    }

    return buffer;
}

void initExt2FS() {
    ASSERT(g_hardDrive2);
    println("Initializing ext2 filesystem");

    SuperBlock* superBlock = readSuperBlock();
    if (!superBlock) {
        return;
    }

    BlockGroupDescriptor* blockGroupDescriptorTable =
        readBlockGroupDescriptorTable(superBlock);
    if (!blockGroupDescriptorTable) {
        return;
    }

    INode* root =
        readINode(superBlock, blockGroupDescriptorTable, EXT2_ROOT_INO);
    if (!root) {
        return;
    }

    if ((root->i_mode & 0xF000) != EXT2_S_IFDIR) {
        panic("ext2: root directory is not a directory");
    }

    uint8_t* contents = readFile(superBlock, root);
    if (!contents) {
        panic("ext2: failed to read root directory");
        return;
    }

    char nameBuffer[256];

    uint16_t offset = 0;
    while (offset < root->size()) {
        DirectoryEntry dirEntry;
        memcpy(&dirEntry, contents + offset, sizeof(dirEntry));
        println("inode: {}", dirEntry.inode);
        println("rec_len: {}", dirEntry.rec_len);
        println("name_len: {}", dirEntry.name_len);
        println("file_type: {}", dirEntry.file_type);

        memcpy(nameBuffer, contents + offset + sizeof(dirEntry),
               dirEntry.name_len);
        nameBuffer[dirEntry.name_len] = '\0';
        println("name: {}", nameBuffer);

        offset += dirEntry.rec_len;
    }
}
