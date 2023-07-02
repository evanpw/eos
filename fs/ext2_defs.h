#include <stdint.h>

namespace ext2 {

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

    uint64_t size() const {
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

}  // namespace ext2
