#include "pipe.h"

#include "scheduler.h"
#include "system.h"

PipePair Pipe::create() {
    estd::shared_ptr<Pipe> pipe(new Pipe);
    estd::shared_ptr<File> reader(new PipeReader(pipe));
    estd::shared_ptr<File> writer(new PipeWriter(pipe));

    return {reader, writer};
}

Pipe::Pipe() {
    _canRead.assign(new Blocker);
    _canWrite.assign(new Blocker);
}

ssize_t Pipe::read(OpenFileDescription&, void* buffer, size_t count) {
    if (count == 0) {
        return 0;
    }

    SpinlockLocker locker(_lock);

    if (_queue.empty() && !_hasWriter) {
        return 0;
    }

    // Wait until some data is available
    while (_queue.empty()) {
        sys.scheduler().sleepThread(_canRead, &_lock);
    }

    bool wasFull = _queue.full();
    ssize_t bytesRead = _queue.read(buffer, count);

    // If the queue was full before and now there's space, wake up writers
    if (wasFull && !_queue.full()) {
        sys.scheduler().wakeThreads(_canWrite);
    }

    return bytesRead;
}

ssize_t Pipe::write(OpenFileDescription&, const void* buffer, size_t count) {
    SpinlockLocker locker(_lock);

    if (!_hasReader) {
        return -EPIPE;
    }

    const byte* ptr = static_cast<const byte*>(buffer);
    size_t bytesWritten = 0;
    while (count > 0) {
        // Wait until there's space to write
        while (_queue.full()) {
            sys.scheduler().sleepThread(_canWrite, &_lock);
        }

        bool wasEmpty = _queue.empty();

        size_t n = _queue.write(ptr, count);
        bytesWritten += n;
        ptr += n;
        count -= n;

        // If the queue was empty before and now there's data, wake up readers
        if (wasEmpty && !_queue.empty()) {
            sys.scheduler().wakeThreads(_canRead);
        }
    }

    return bytesWritten;
}
