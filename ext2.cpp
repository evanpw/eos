#include "ext2.h"

#include "ide.h"
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
};

static_assert(sizeof(SuperBlock) == 1024);

void initExt2FS() {
    ASSERT(g_hardDrive2);
    println("Initializing ext2 filesystem");

    // The ext2 superblock is always 1024 bytes at LBA 2 (offset 1024)
    SuperBlock* superBlock = new SuperBlock;
    if (!g_hardDrive2->readSectors(superBlock, 2, 1024)) {
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
}
