#include "ext2_file.h"

#include "api/dirent.h"
#include "api/errno.h"
#include "estd/new.h"  // IWYU pragma: keep
#include "estd/utility.h"

Ext2File::Ext2File(Ext2FileSystem& fs, estd::unique_ptr<ext2::Inode> inode)
: _fs(fs), _inode(estd::move(inode)) {}

ssize_t Ext2File::read(OpenFileDescription& fd, void* buffer, size_t count) {
    ssize_t bytesRead =
        _fs.readFromFile(*_inode, reinterpret_cast<uint8_t*>(buffer), count, fd.offset);
    if (bytesRead > 0) {
        // TODO: file descriptor needs locking
        fd.offset += bytesRead;
    }

    return bytesRead;
}

ssize_t Ext2File::write(OpenFileDescription& /*fd*/, const void* /*buffer*/,
                        size_t /*count*/) {
    // We don't have a writeable filesystem yet
    return -EBADF;
}

ssize_t Ext2File::readDir(OpenFileDescription& /*fd*/, void* buffer, size_t count) {
    // TODO: what should be done about fd.offset?

    // Make sure that current location is a directory
    if ((_inode->mode & 0xF000) != ext2::S_IFDIR) {
        return -ENOTDIR;
    }

    size_t size = _inode->size();
    Buffer dirBuffer(size);
    if (!_fs.readFullFile(*_inode, dirBuffer.get())) {
        return -EIO;
    }

    // Copy the directory entries from  dirBuffer to buffer, converting the structure as
    // we go
    uint8_t* pbuffer = reinterpret_cast<uint8_t*>(buffer);
    uint64_t inOffset = 0;
    uint64_t outOffset = 0;
    while (inOffset < size) {
        ext2::DirectoryEntry* dirEntry =
            reinterpret_cast<ext2::DirectoryEntry*>(&dirBuffer[inOffset]);

        size_t outEntrySize = sizeof(dirent) + dirEntry->name_len + 1;
        if (outOffset + outEntrySize > count) {
            // EINVAL tells the caller that the buffer was too small
            return -EINVAL;
        }

        // TODO: alignment issues with dirent
        dirent* outEntry = new (&pbuffer[outOffset]) dirent;
        outEntry->d_ino = dirEntry->inode;
        outEntry->d_reclen = outEntrySize;
        outEntry->d_type = dirEntry->file_type;
        memcpy(outEntry->d_name, dirEntry->name, dirEntry->name_len);
        outEntry->d_name[dirEntry->name_len] = '\0';

        inOffset += dirEntry->rec_len;
        outOffset += outEntrySize;
    }

    return outOffset;
}
