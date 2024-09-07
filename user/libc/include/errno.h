// https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/errno.h.html
#pragma once

#define ENOENT 2   // No such file or directory
#define EIO 5      // I/O error
#define EBADF 9    // Invalid file descriptor
#define EMFILE 24  // Too many open files

// TODO: should be thread-local?
extern "C" int errno;
