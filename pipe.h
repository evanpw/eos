#pragma once

#include "estd/byte_queue.h"
#include "estd/shared_ptr.h"
#include "file.h"
#include "scheduler.h"
#include "spinlock.h"
#include "units.h"

struct PipePair {
    estd::shared_ptr<File> reader;
    estd::shared_ptr<File> writer;
};

class Pipe {
public:
    static constexpr size_t BUFFER_SIZE = 4 * KiB;

    static PipePair create();

    ssize_t read(OpenFileDescription& fd, void* buffer, size_t count);
    ssize_t write(OpenFileDescription& fd, const void* buffer, size_t count);

    void readerClosed() {
        SpinlockLocker locker(_lock);
        _hasReader = false;
    }

    void writerClosed() {
        SpinlockLocker locker(_lock);
        _hasWriter = false;
    }

private:
    Pipe();

    // Not copyable / moveable
    Pipe(const Pipe&) = delete;
    Pipe& operator=(const Pipe&) = delete;

    Spinlock _lock;
    ByteQueue<BUFFER_SIZE> _queue;

    bool _hasReader = true;
    bool _hasWriter = true;

    estd::shared_ptr<Blocker> _canRead;
    estd::shared_ptr<Blocker> _canWrite;
};

class PipeReader : public File {
public:
    PipeReader(const estd::shared_ptr<Pipe>& parent) : _parent(parent) {}
    ~PipeReader() { _parent->readerClosed(); }

    ssize_t read(OpenFileDescription& fd, void* buffer, size_t count) override {
        return _parent->read(fd, buffer, count);
    }

    ssize_t write(OpenFileDescription&, const void*, size_t) override { return -EBADF; }

    virtual bool isPipe() const override { return true; }

private:
    estd::shared_ptr<Pipe> _parent;
};

class PipeWriter : public File {
public:
    PipeWriter(const estd::shared_ptr<Pipe>& parent) : _parent(parent) {}
    ~PipeWriter() { _parent->writerClosed(); }

    ssize_t read(OpenFileDescription&, void*, size_t) override { return -EBADF; }

    ssize_t write(OpenFileDescription& fd, const void* buffer, size_t count) override {
        return _parent->write(fd, buffer, count);
    }

    virtual bool isPipe() const override { return true; }

private:
    estd::shared_ptr<Pipe> _parent;
};
