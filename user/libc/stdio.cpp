#include "stdio.h"

#include "stdlib.h"
#include "syscall.h"
#include "unistd.h"

FILE _IO_stdin;
FILE _IO_stdout;
FILE _IO_stderr;

static void initStream(FILE* stream, int fd, int flags) {
    stream->fd = fd;
    stream->flags = flags;
    stream->buffer = (unsigned char*)malloc(BUFSIZ);
    stream->bufferSize = BUFSIZ;
    stream->readTail = 0;
    stream->readHead = 0;
}

static bool canRead(FILE* stream) { return stream->flags & _IO_READ; }
static bool canWrite(FILE* stream) { return stream->flags & _IO_WRITE; }
static void setError(FILE* stream) { stream->flags |= _IO_ERR; }
static void setEOF(FILE* stream) { stream->flags |= _IO_EOF; }
static bool isBuffered(FILE* stream) { return (stream->flags & _IO_BUFFERING) != _IONBF; }
static bool isLineBuffered(FILE* stream) { return stream->flags & _IOLBF; }

static ssize_t doRead(FILE* stream, void* dest, size_t size) {
    int64_t ret = syscall(SYS_read, stream->fd, dest, size);
    if (ret <= 0) {
        if (ret == 0) {
            setEOF(stream);
        } else {
            setError(stream);
            errno = -ret;
        }

        return -1;
    }

    return ret;
}

static bool doWrite(FILE* stream, void* vsrc, size_t size) {
    unsigned char* src = static_cast<unsigned char*>(vsrc);

    while (size > 0) {
        int64_t ret = syscall(SYS_write, stream->fd, src, size);
        if (ret < 0) {
            setError(stream);
            errno = -ret;
            return false;
        }

        src += ret;
        size -= ret;
    }

    return true;
}

static void flushReadBuffer(FILE* stream) {
    stream->readHead = 0;
    stream->readTail = 0;
}

static bool flushWriteBuffer(FILE* stream) {
    bool result = doWrite(stream, stream->buffer, stream->writeTail);
    stream->writeTail = 0;
    return result;
}

void initStdio() {
    // stdin and stdout are fully buffered if not ttys; stderr is always unbuffered
    int stdinBuffering = isatty(STDIN_FILENO) ? _IONBF : _IOFBF;
    int stdoutBuffering = isatty(STDOUT_FILENO) ? _IOLBF : _IOFBF;
    int stderrBuffering = _IONBF;

    initStream(stdin, STDIN_FILENO, _IO_READ | stdinBuffering);
    initStream(stdout, STDOUT_FILENO, _IO_WRITE | stdoutBuffering);
    initStream(stderr, STDERR_FILENO, _IO_WRITE | stderrBuffering);
}

int fileno(FILE* stream) { return stream->fd; }
int feof(FILE* stream) { return stream->flags & _IO_EOF; }
int ferror(FILE* stream) { return stream->flags & _IO_ERR; }
void clearerr(FILE* stream) { stream->flags &= ~(_IO_ERR | _IO_EOF); }

// https://pubs.opengroup.org/onlinepubs/9799919799/functions/fclose.html
int fclose(FILE* stream) {
    if (fflush(stream) == EOF) {
        return EOF;
    }

    if (!(stream->flags & _IO_USER_BUF)) {
        free(stream->buffer);
    }

    return try_syscall(SYS_close, stream->fd);
}

// https://pubs.opengroup.org/onlinepubs/9799919799/functions/fgetc.html
int fgetc(FILE* stream) {
    if (feof(stream)) {
        return EOF;
    }

    if (!canRead(stream)) {
        setError(stream);
        return EOF;
    }

    // Unbuffered case is easy
    if (!isBuffered(stream)) {
        unsigned char c;
        if (doRead(stream, &c, 1) < 0) {
            return EOF;
        }

        return c;
    }

    // TODO: flush writes if necessary (write followed by read with no flush is UB
    // according to the spec)

    // Satisfy the request from the buffer if possible
    if (stream->readHead < stream->readTail) {
        return stream->buffer[stream->readHead++];
    }

    // Otherwise, refill the buffer from the file descriptor
    flushReadBuffer(stream);
    ssize_t bytesRead = doRead(stream, stream->buffer, stream->bufferSize);
    if (bytesRead < 0) {
        return EOF;
    }

    stream->readTail = bytesRead;

    return stream->buffer[stream->readHead++];
}

// https://pubs.opengroup.org/onlinepubs/9799919799/functions/getc.html
int getc(FILE* stream) { return fgetc(stream); }

// https://pubs.opengroup.org/onlinepubs/9799919799/functions/getchar.html
int getchar() { return getc(stdin); }

// https://pubs.opengroup.org/onlinepubs/9799919799/functions/fread.html
size_t fread(void* vptr, size_t size, size_t nitems, FILE* stream) {
    unsigned char* ptr = static_cast<unsigned char*>(vptr);

    for (size_t i = 0; i < nitems; i++) {
        for (size_t j = 0; j < size; j++) {
            int c = fgetc(stream);
            if (c == EOF) {
                return i;
            }

            *ptr++ = c;
        }
    }

    return nitems;
}

// https://pubs.opengroup.org/onlinepubs/9799919799/functions/fputc.html
int fputc(int ic, FILE* stream) {
    unsigned char c = ic;

    if (!canWrite(stream)) {
        setError(stream);
        return EOF;
    }

    // Unbuffered case is easy
    if (!isBuffered(stream)) {
        if (!doWrite(stream, &c, 1)) {
            return EOF;
        }

        return ic;
    }

    // A write following a read with no seek in between is UB, but we choose to just flush
    // the read buffer here to avoid problems
    flushReadBuffer(stream);

    // Since we flush the buffer once full, we should never have a full buffer here
    // assert(stream->writeTail < stream->bufferSize);

    // Insert the character into the buffer
    stream->buffer[stream->writeTail++] = c;

    // If the buffer is full, or this is a line-buffered stream and we just wrote a
    // newline, then write the buffer to the file descriptor
    bool shouldFlush = stream->writeTail == stream->bufferSize;
    shouldFlush |= (isLineBuffered(stream) && c == '\n');
    if (shouldFlush) {
        flushWriteBuffer(stream);
    }

    return ic;
}

// https://pubs.opengroup.org/onlinepubs/9799919799/functions/putc.html
int putc(int c, FILE* stream) { return fputc(c, stream); }

// https://pubs.opengroup.org/onlinepubs/9799919799/functions/putchar.html
int putchar(int c) { return putc(c, stdout); }

// https://pubs.opengroup.org/onlinepubs/9799919799/functions/fwrite.html
size_t fwrite(const void* ptr, size_t size, size_t nitems, FILE* stream) {
    const unsigned char* src = static_cast<const unsigned char*>(ptr);

    for (size_t i = 0; i < nitems; i++) {
        for (size_t j = 0; j < size; j++) {
            if (fputc(*src++, stream) == EOF) {
                return i;
            }
        }
    }

    return nitems;
}

// https://pubs.opengroup.org/onlinepubs/9799919799/functions/setvbuf.html
int setvbuf(FILE* stream, char* buf, int type, size_t size) {
    if (type != _IOFBF && type != _IOLBF && type != _IONBF) {
        errno = EINVAL;
        return -1;
    }

    stream->flags = (stream->flags & ~_IO_BUFFERING) | type;

    if (type == _IONBF || size == 0) return 0;

    if (buf) {
        stream->buffer = reinterpret_cast<unsigned char*>(buf);
        stream->bufferSize = size;
        stream->flags |= _IO_USER_BUF;
    } else {
        stream->buffer = (unsigned char*)malloc(size);
        stream->bufferSize = size;
        stream->flags &= ~_IO_USER_BUF;
    }

    return 0;
}

// https://pubs.opengroup.org/onlinepubs/9799919799/functions/setbuf.html
void setbuf(FILE* stream, char* buf) {
    setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}

// https://pubs.opengroup.org/onlinepubs/9799919799/functions/fflush.html
int fflush(FILE* stream) {
    if (!stream) {
        // TODO: flush all streams
        return EOF;
    }

    if (stream->writeTail != 0) {
        if (!flushWriteBuffer(stream)) {
            return EOF;
        }
    }

    // TODO: sync read position
    flushReadBuffer(stream);

    return 0;
}

// https://pubs.opengroup.org/onlinepubs/9799919799/functions/fputs.html
int fputs(const char* s, FILE* stream) {
    while (*s) {
        if (fputc(*s++, stream) == EOF) {
            return EOF;
        }
    }

    return 0;
}

// https://pubs.opengroup.org/onlinepubs/9799919799/functions/puts.html
int puts(const char* s) {
    if (fputs(s, stdout) == EOF) {
        return EOF;
    }

    if (fputc('\n', stdout) == EOF) {
        return EOF;
    }

    return 0;
}
