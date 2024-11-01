#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _IO_FILE {
    int fd;
    int flags;

    unsigned char* buffer;
    size_t bufferSize;

    size_t readHead;
    size_t readTail;

    size_t writeTail;
};

extern struct _IO_FILE _IO_stdin;
extern struct _IO_FILE _IO_stdout;
extern struct _IO_FILE _IO_stderr;

// For the flags field in _IO_FILE
enum {
    _IO_READ = 1,
    _IO_WRITE = 2,
    _IO_EOF = 4,
    _IO_ERR = 8,
    _IO_USER_BUF = 16,

    _IOFBF = 16,
    _IOLBF = 32,
    _IONBF = 64,
    _IO_BUFFERING = _IOFBF | _IOLBF | _IONBF,
};

// Really should be called _IO_init or something
void initStdio();

#ifdef __cplusplus
}
#endif
