#include "dirent.h"

#include "errno.h"
#include "estd/new.h"  // IWYU pragma: keep
#include "fcntl.h"
#include "syscall.h"

static bool isValidDir(DIR* dir) {
    // return dir && dir->buffer && dir->bufferSize != 0;
    return dir && dir->bufferSize != 0;
}

int closedir(DIR* dir) {
    if (!isValidDir(dir)) {
        errno = -EBADF;
        return -1;
    }

    // delete[] dir->buffer;
    // delete dir;
    return 0;
}

// TODO: dynamic memory allocation in user space
DIR theDir;

DIR* opendir(const char* name) {
    int fd = open(name, 0);
    if (fd == -1) {
        return nullptr;
    }

    int64_t result = __syscall(SYS_read_dir, fd, (uint64_t)theDir.buffer, 4096);

    if (result >= 0) {
        theDir.offset = 0;
        theDir.bufferSize = result;
        return &theDir;
    }

    if (result == -EINVAL) {
        // The read_dir syscall returns EINVAL if the buffer is too small
        errno = EFBIG;
        return nullptr;
    }

    // Any other that occurs in the syscall is just returned to the caller
    errno = -result;
    return nullptr;

    /*
    DIR* dir = new DIR;

    // Start with a size of 4 KB and grow if necessary
    dir->bufferSize = 4096;
    while (true) {
        dir->buffer = new uint8_t[dir->bufferSize];

        int64_t result =
            __syscall(SYS_read_dir, fd, (uint64_t)dir->buffer, dir->bufferSize);

        if (result >= 0) {
            return dir;
        }

        if (result == -EINVAL) {
            // The read_dir syscall returns EINVAL if the buffer is too small
            delete[] dir->buffer;
            dir->bufferSize *= 2;
            continue;
        }

        // Any other that occurs in the syscall is just returned to the caller
        delete[] dir->buffer;
        delete dir;
        errno = -result;
        return nullptr;
    }
    */
}

dirent* readdir(DIR* dir) {
    // Not a valid open directory stream
    if (!isValidDir(dir)) {
        errno = -EBADF;
        return nullptr;
    }

    // End of stream
    if (dir->offset == dir->bufferSize) {
        // POSIX says we shouldn't reset errno to 0 here even though that makes it
        // impossible to distinguish between end-of-stream and an error
        return nullptr;
    }

    // Current position of the directory stream is invalid
    if (dir->offset + sizeof(dirent) > dir->bufferSize) {
        errno = -ENOENT;
        return nullptr;
    }

    dirent* entry = (dirent*)&dir->buffer[dir->offset];

    // Partial record
    if (dir->offset + entry->d_reclen > dir->bufferSize) {
        errno = -ENOENT;
        return nullptr;
    }

    dir->offset += entry->d_reclen;
    return entry;
}

void seekdir(DIR* dir, long loc) { dir->offset = loc; }
long telldir(DIR* dir) { return dir->offset; }
