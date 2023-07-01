#include "process.h"

#include "errno.h"
#include "file.h"

int Process::open(File& file) {
    // Find next available fd
    for (size_t i = 0; i < RLIMIT_NOFILE; ++i) {
        if (!openFiles[i]) {
            openFiles[i] = file.open();
            return i;
        }
    }

    return -EMFILE;
}

int Process::close(int fd) {
    if (fd < 0 || fd >= RLIMIT_NOFILE || !openFiles[fd]) {
        return -EBADF;
    }

    openFiles[fd].clear();
    return 0;
}
