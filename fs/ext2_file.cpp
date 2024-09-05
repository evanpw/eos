#include "ext2_file.h"

#include "errno.h"
#include "estd/stdlib.h"

Ext2File::Ext2File(Ext2FileSystem& fs, OwnPtr<ext2::Inode> inode)
: _fs(fs), _inode(move(inode)) {}

ssize_t Ext2File::read(OpenFileDescription& fd, void* buffer, size_t count) {
    ssize_t bytesRead =
        _fs.readFromFile(*_inode, reinterpret_cast<uint8_t*>(buffer), count, fd.offset);
    if (bytesRead > 0) {
        // TODO: file descriptor needs locking
        fd.offset += bytesRead;
    }

    return bytesRead;
}

ssize_t Ext2File::write(OpenFileDescription& fd, const void* buffer, size_t count) {
    // We don't have a writeable filesystem yet
    return -EBADF;
}
