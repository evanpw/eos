#pragma once
#include <stddef.h>
#include <sys/types.h>

struct File;

// The kernel data structure that a usermode file descriptor points to
struct OpenFileDescription {
    File& file;
};

// Something that supports open/close and read / write. Could be a device, a
// file on disk, a socket, a pipe, or a bunch of other things. It may or may
// not be accessible via the file system
struct File {
    virtual OpenFileDescription* open();
    virtual void close(){};
    virtual ssize_t read(OpenFileDescription& fd, void* buffer,
                         size_t count) = 0;
    virtual ssize_t write(OpenFileDescription& fd, const void* buffer,
                          size_t count) = 0;
};
