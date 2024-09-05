#pragma once
#include "estd/ownptr.h"
#include "file.h"
#include "fs/ext2.h"
#include "fs/ext2_defs.h"

class Ext2File : public File {
public:
    Ext2File(Ext2FileSystem& fs, OwnPtr<ext2::Inode> inode);

    ssize_t read(OpenFileDescription& fd, void* buffer, size_t count) override;
    ssize_t write(OpenFileDescription& fd, const void* buffer, size_t count) override;

private:
    Ext2FileSystem& _fs;
    OwnPtr<ext2::Inode> _inode;
};
