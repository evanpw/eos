#pragma once
#include <stdint.h>

namespace ext2 {

// s_magic
static constexpr uint16_t SUPER_MAGIC = 0xEF53;

enum SState : uint16_t {
    VALID_FS = 1,
    ERROR_FS = 2,
};

enum SRevLevel : uint32_t {
    GOOD_OLD_REV = 0,
    DYNAMIC_REV = 1,
};

enum SErrors : uint16_t {
    ERRORS_CONTINUE = 1,
    ERRORS_RO = 2,
    ERRORS_PANIC = 3,
};

enum SCreatorOS : uint32_t {
    OS_LINUX = 0,
    OS_HURD = 1,
    OS_MASIX = 2,
    OS_FREEBSD = 3,
    OS_LITES = 4,
};

enum SFeatureCompat : uint32_t {
    FEATURE_COMPAT_DIR_PREALLOC = 0x0001,
    FEATURE_COMPAT_IMAGIC_INODES = 0x0002,
    FEATURE_COMPAT_HAS_JOURNAL = 0x0004,
    FEATURE_COMPAT_EXT_ATTR = 0x0008,
    FEATURE_COMPAT_RESIZE_INO = 0x0010,
    FEATURE_COMPAT_DIR_INDEX = 0x0020,
};

enum SFeatureIncompat : uint32_t {
    FEATURE_INCOMPAT_COMPRESSION = 0x0001,
    FEATURE_INCOMPAT_FILETYPE = 0x0002,
    FEATURE_INCOMPAT_RECOVER = 0x0004,
    FEATURE_INCOMPAT_JOURNAL_DEV = 0x0008,
    FEATURE_INCOMPAT_META_BG = 0x0010,
};

enum SFeatureROCompat : uint32_t {
    FEATURE_RO_COMPAT_SPARSE_SUPER = 0x0001,
    FEATURE_RO_COMPAT_LARGE_FILE = 0x0002,
    FEATURE_RO_COMPAT_BTREE_DIR = 0x0004,
};

enum ReservedInodes : uint32_t {
    BAD_INO = 1,
    ROOT_INO = 2,
    ACL_IDX_INO = 3,
    ACL_DATA_INO = 4,
    BOOT_LOADER_INO = 5,
    UNDEL_DIR_INO = 6,
};

struct __attribute__((packed)) SuperBlock {
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t r_blocks_count;
    uint32_t free_blocks_count;
    uint32_t free_inodes_count;
    uint32_t first_data_block;
    uint32_t log_block_size;
    uint32_t log_frag_size;  // not used
    uint32_t blocks_per_group;
    uint32_t frags_per_group;  // not used
    uint32_t inodes_per_group;
    uint32_t mtime;
    uint32_t wtime;
    uint16_t mnt_count;
    uint16_t max_mnt_count;
    uint16_t magic;
    SState state;
    SErrors errors;
    uint16_t minor_rev_level;
    uint32_t lastcheck;
    uint32_t checkinterval;
    SCreatorOS creator_os;
    SRevLevel rev_level;
    uint16_t def_resuid;
    uint16_t def_resgid;
    uint32_t first_ino;
    uint16_t inode_size;
    uint16_t block_group_nr;
    SFeatureCompat feature_compat;
    SFeatureIncompat feature_incompat;
    SFeatureROCompat feature_ro_compat;
    uint8_t uuid[16];
    char volume_name[16];
    char last_mounted[64];
    uint32_t algo_bitmap;
    uint8_t prealloc_blocks;
    uint8_t prealloc_dir_blocks;
    uint16_t _alignment;
    uint8_t journal_uuid[16];
    uint32_t journal_inum;
    uint32_t journal_dev;
    uint32_t last_orphan;
    uint32_t hash_seed[4];
    uint8_t def_hash_version;
    uint8_t _padding[3];
    uint32_t default_mount_options;
    uint32_t first_meta_bg;
    uint8_t unused[760];
};

static_assert(sizeof(SuperBlock) == 1024);

struct __attribute__((packed)) BlockGroupDescriptor {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t free_blocks_count;
    uint16_t free_inodes_count;
    uint16_t used_dirs_count;
    uint16_t pad;
    char reserved[12];
};

static_assert(sizeof(BlockGroupDescriptor) == 32);

enum IMode : uint16_t {
    // File format
    S_IFSOCK = 0xC000,
    S_IFLNK = 0xA000,
    S_IFREG = 0x8000,
    S_IFBLK = 0x6000,
    S_IFDIR = 0x4000,
    S_IFCHR = 0x2000,
    S_IFIFO = 0x1000,
    // User / group override
    S_ISUID = 0x0800,
    S_ISGID = 0x0400,
    S_ISVTX = 0x0200,
    // Access control
    S_IRUSR = 0x0100,
    S_IWUSR = 0x0080,
    S_IXUSR = 0x0040,
    S_IRGRP = 0x0020,
    S_IWGRP = 0x0010,
    S_IXGRP = 0x0008,
    S_IROTH = 0x0004,
    S_IWOTH = 0x0002,
    S_IXOTH = 0x0001,
};

struct __attribute__((packed)) Inode {
    IMode mode;
    uint16_t uid;
    uint32_t size32;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t gid;
    uint16_t links_count;
    uint32_t blocks;
    uint32_t flags;
    uint32_t osd1;
    uint32_t block[15];
    uint32_t generation;
    uint32_t file_acl;
    uint32_t dir_acl;
    uint32_t faddr;
    char osd2[12];

    uint64_t size() const {
        if ((mode & 0xF000) != S_IFREG) {
            return (static_cast<uint64_t>(dir_acl) << 32) | size32;
        } else {
            return size32;
        }
    }

    bool isDirectory() const { return (mode & 0xF000) == S_IFDIR; }
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

}  // namespace ext2
