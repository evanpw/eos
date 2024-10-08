// IWYU pragma: always_keep
// https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/errno.h.html
#pragma once

#define ENOENT 2            // No such file or directory
#define EIO 5               // I/O error
#define EBADF 9             // Invalid file descriptor
#define EINVAL 22           // Invalid argument
#define EMFILE 24           // Too many open files
#define ENOTDIR 20          // Not a directory
#define EFBIG 27            // File too large
#define ENOMEM 12           // Out of memory
#define ENAMETOOLONG 36     // File name too long
#define ECHILD 10           // No child processes
#define ENOSYS 38           // Function not implemented
#define EAFNOSUPPORT 97     // Address family not supported
#define EPROTONOSUPPORT 93  // Protocol not supported
#define EPROTOTYPE 91       // Protocol wrong type for socket
#define ENOTSOCK 88         // Socket operation on non-socket
#define ECONNREFUSED 111    // Connection refused
#define ENOTCONN 107        // Socket is not connected
#define EPIPE 32            // Broken pipe
#define EADDRINUSE 98       // Address already in use
