#include "file.h"

#include <stdio.h>

estd::shared_ptr<OpenFileDescription> OpenFileDescription::create(
    const estd::shared_ptr<File>& file) {
    return estd::make_shared<OpenFileDescription>(file);
}

off_t OpenFileDescription::lseek(off_t offset, int whence) {
    if (!file->isSeekable()) {
        return -ESPIPE;
    }

    off_t current = this->offset;
    size_t fileSize = file->size();

    off_t newOffset;
    if (whence == SEEK_SET) {
        newOffset = offset;
    } else if (whence == SEEK_CUR) {
        newOffset = current + offset;
    } else if (whence == SEEK_END) {
        newOffset = fileSize + offset;
    } else {
        return -EINVAL;
    }

    if (newOffset < 0 || (size_t)newOffset > fileSize) {
        return -EINVAL;
    }

    this->offset = newOffset;
    return newOffset;
}
