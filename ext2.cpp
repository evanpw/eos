#include "ext2.h"

#include "ide.h"
#include "panic.h"
#include "units.h"

struct __attribute__((packed)) SuperBlock {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;

    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
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

void initExt2FS() {
    ASSERT(g_hardDrive2);
    println("Initializing ext2 filesystem");

    // The ext2 superblock is always 1024 bytes at LBA 2 (offset 1024)
    SuperBlock* superBlock = new SuperBlock;
    if (!g_hardDrive2->readSectors(superBlock, 2, 1024)) {
        panic("Failed to read superblock");
    }

    println("magic: {:04X}", superBlock->s_magic);
    ASSERT(superBlock->s_magic == 0xEF53);

    print("major revision: ");
    if (superBlock->s_rev_level == 0) {
        println("EXT2_GOOD_OLD_REV");
        ASSERT(false);
    } else if (superBlock->s_rev_level == 1) {
        println("EXT2_DYNAMIC_REV");
    } else {
        ASSERT(false);
    }

    println("minor revision: {}", superBlock->s_minor_rev_level);

    println("number of inodes: {}", superBlock->s_inodes_count);
    println("number of blocks: {}", superBlock->s_blocks_count);
    println("number of reserved blocks: {}", superBlock->s_r_blocks_count);
    println("number of free blocks: {}", superBlock->s_free_blocks_count);
    println("number of free inodes: {}", superBlock->s_free_inodes_count);

    println("first data block: {}", superBlock->s_first_data_block);
    if (superBlock->s_log_block_size > 0) {
        ASSERT(superBlock->s_first_data_block == 0);
    } else {
        ASSERT(superBlock->s_first_data_block == 1);
    }

    println("block size: {} KiB", (1024 << superBlock->s_log_block_size) / KiB);
    ASSERT((1024UL << superBlock->s_log_block_size) >= 512);
    ASSERT((1024UL << superBlock->s_log_block_size) <= PAGE_SIZE);

    println("fragment size: {} KiB",
            (1024UL << superBlock->s_log_frag_size) / KiB);
    ASSERT(superBlock->s_log_frag_size >= superBlock->s_log_block_size);

    println("blocks per group: {}", superBlock->s_blocks_per_group);
    println("fragments per group: {}", superBlock->s_frags_per_group);

    println("inodes per group: {}", superBlock->s_inodes_per_group);
    ASSERT(superBlock->s_inodes_per_group %
               ((1024 << superBlock->s_log_block_size) /
                superBlock->s_inode_size) ==
           0);

    println("last mount time: {}", superBlock->s_mtime);
    println("last write time: {}", superBlock->s_wtime);
    println("mounts since last verification: {}", superBlock->s_mnt_count);
    println("max mounts before verification: {}", superBlock->s_max_mnt_count);

    print("filesystem state: ");
    if (superBlock->s_state == 1) {
        println("EXT2_VALID_FS");
    } else if (superBlock->s_state == 2) {
        println("EXT2_ERROR_FS");
    } else {
        ASSERT(false);
    }

    print("error handling: ");
    if (superBlock->s_errors == 1) {
        println("EXT2_ERRORS_CONTINUE");
    } else if (superBlock->s_errors == 2) {
        println("EXT2_ERRORS_RO");
    } else if (superBlock->s_errors == 3) {
        println("EXT2_ERRORS_PANIC");
    } else {
        println("{}", superBlock->s_errors);
    }

    println("last filesystem check time: {}", superBlock->s_lastcheck);
    println("max interval between filesystem checks: {}",
            superBlock->s_checkinterval);

    print("creator os: ");
    if (superBlock->s_creator_os == 0) {
        println("EXT2_OS_LINUX");
    } else if (superBlock->s_creator_os == 1) {
        println("EXT2_OS_HURD");
    } else if (superBlock->s_creator_os == 2) {
        println("EXT2_OS_MASIX");
    } else if (superBlock->s_creator_os == 3) {
        println("EXT2_OS_FREEBSD");
    } else if (superBlock->s_creator_os == 4) {
        println("EXT2_OS_LITES");
    } else {
        ASSERT(false);
    }

    println("default userid for reserved blocks: {}", superBlock->s_def_resuid);
    println("default group id for reserved blocks: {}",
            superBlock->s_def_resgid);

    println("first inode index: {}", superBlock->s_first_ino);

    println("node size: {}", superBlock->s_inode_size);
    ASSERT((superBlock->s_inode_size & (superBlock->s_inode_size - 1)) == 0);
    ASSERT(superBlock->s_inode_size <= (1024 << superBlock->s_log_block_size));

    println("superblock block group number: {}", superBlock->s_block_group_nr);

    print("compatible features:", superBlock->s_feature_compat);
    if (superBlock->s_feature_compat & 0x0001) {
        print(" EXT2_FEATURE_COMPAT_DIR_PREALLOC");
    }
    if (superBlock->s_feature_compat & 0x0002) {
        print(" EXT2_FEATURE_COMPAT_IMAGIC_INODES");
    }
    if (superBlock->s_feature_compat & 0x0004) {
        print(" EXT2_FEATURE_COMPAT_HAS_JOURNAL");
    }
    if (superBlock->s_feature_compat & 0x0008) {
        print(" EXT2_FEATURE_COMPAT_EXT_ATTR");
    }
    if (superBlock->s_feature_compat & 0x0010) {
        print(" EXT2_FEATURE_COMPAT_RESIZE_INO");
    }
    if (superBlock->s_feature_compat & 0x0020) {
        print(" EXT2_FEATURE_COMPAT_DIR_INDEX");
    }
    println("");

    print("incompatible features:");
    if (superBlock->s_feature_incompat & 0x0001) {
        print(" EXT2_FEATURE_INCOMPAT_COMPRESSION");
    }
    if (superBlock->s_feature_incompat & 0x0002) {
        print(" EXT2_FEATURE_INCOMPAT_FILETYPE");
    }
    if (superBlock->s_feature_incompat & 0x0004) {
        print(" EXT2_FEATURE_INCOMPAT_RECOVER");
    }
    if (superBlock->s_feature_incompat & 0x0008) {
        print(" EXT2_FEATURE_JOURNAL_DEV");
    }
    if (superBlock->s_feature_incompat & 0x0010) {
        print(" EXT2_FEATURE_META_BG");
    }
    println("");
    ASSERT((superBlock->s_feature_incompat & ~uint16_t(0x0002)) == 0);

    print("write-incompatible features:");
    if (superBlock->s_feature_ro_compat & 0x0001) {
        print(" EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER");
    }
    if (superBlock->s_feature_ro_compat & 0x0002) {
        print(" EXT2_FEATURE_RO_COMPAT_LARGE_FILE");
    }
    if (superBlock->s_feature_ro_compat & 0x0004) {
        print(" EXT2_FEATURE_RO_COMPAT_BTREE_DIR");
    }
    println("");

    // println("s_uuid: {}", superBlock->s_uuid);
    println("s_volume_name: {}", superBlock->s_volume_name);
    println("s_last_mounted: {}", superBlock->s_last_mounted);
    println("s_algo_bitmap: {}", superBlock->s_algo_bitmap);
    println("s_prealloc_blocks: {}", superBlock->s_prealloc_blocks);
    println("s_prealloc_dir_blocks: {}", superBlock->s_prealloc_dir_blocks);
    // println("s_journal_uuid: {}", superBlock->s_journal_uuid);
    println("s_journal_inum: {}", superBlock->s_journal_inum);
    println("s_journal_dev: {}", superBlock->s_journal_dev);
    println("s_last_orphan: {}", superBlock->s_last_orphan);
    // println("s_hash_seed[4]: {}", superBlock->s_hash_seed[4]);
    println("s_def_hash_version: {}", superBlock->s_def_hash_version);
    println("s_default_mount_options: {}", superBlock->s_default_mount_options);
    println("s_first_meta_bg: {}", superBlock->s_first_meta_bg);
}
