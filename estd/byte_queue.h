#pragma once
#include <stddef.h>
#include <string.h>

#include "estd/assertions.h"
#include "estd/stddef.h"
#include "klibc.h"

template <size_t N>
class ByteQueue {
public:
    ByteQueue() : _head(&_data[0]), _tail(&_data[0]) {}

    size_t write(const void* data, size_t dataSize) {
        if (dataSize == 0 || full()) return 0;

        size_t bytesWritten = 0;
        const byte* dataPtr = static_cast<const byte*>(data);

        // First try to write up to the end of the storage array
        if (_tail >= _head) {
            // How many contiguous bytes can we write?
            size_t roomLeft = N - tailIdx();
            if (headIdx() != 0) {
                roomLeft += 1;
            }

            size_t sizeToWrite = min<size_t>(dataSize, roomLeft);
            memcpy(_tail, dataPtr, sizeToWrite);

            bytesWritten += sizeToWrite;
            dataPtr += sizeToWrite;
            dataSize -= sizeToWrite;
            _tail = increment(_tail, sizeToWrite);

            if (dataSize == 0 || full()) return bytesWritten;
        }

        ASSERT(_tail < _head);

        // Then try to write up to one byte before the head pointer
        size_t spaceRemaining = (headIdx() - 1) - tailIdx();
        if (spaceRemaining > 0) {
            size_t sizeToWrite = min<size_t>(dataSize, spaceRemaining);
            memcpy(_tail, dataPtr, sizeToWrite);

            bytesWritten += sizeToWrite;
            _tail = increment(_tail, sizeToWrite);
        }

        return bytesWritten;
    }

    size_t read(void* buffer, size_t bufferSize) {
        if (bufferSize == 0 || empty()) return 0;

        size_t bytesRead = 0;
        byte* bufferPtr = static_cast<byte*>(buffer);

        // First try to read up to the end of the storage array
        if (_tail < _head) {
            // How many contiguous bytes can we read?
            size_t bytesToEnd = N + 1 - headIdx();

            size_t sizeToRead = min<size_t>(bufferSize, bytesToEnd);
            memcpy(bufferPtr, _head, sizeToRead);

            bytesRead += sizeToRead;
            bufferPtr += sizeToRead;
            bufferSize -= sizeToRead;

            if (sizeToRead == bytesToEnd) {
                _head = &_data[0];
            } else {
                _head += sizeToRead;
            }

            if (bufferSize == 0) return bytesRead;
        }

        ASSERT(_head <= _tail);

        // Then try to read up to the tail pointer
        size_t spaceRemaining = tailIdx() - headIdx();
        if (spaceRemaining > 0) {
            size_t sizeToRead = min<size_t>(bufferSize, spaceRemaining);
            memcpy(bufferPtr, _head, sizeToRead);

            bytesRead += sizeToRead;
            _head += sizeToRead;
        }

        return bytesRead;
    }

    size_t size() const {
        if (_tail >= _head) {
            return _tail - _head;
        } else {
            return (N + 1) - (_head - _tail);
        }
    }

    bool empty() const { return size() == 0; }
    bool full() const { return size() == N; }
    operator bool() const { return !empty(); }

private:
    size_t headIdx() { return _head - &_data[0]; }
    size_t tailIdx() { return _tail - &_data[0]; }

    byte* increment(byte* ptr, size_t count) {
        ASSERT(count <= N);
        ptr += count;

        if (ptr == &_data[N + 1]) {
            ptr = &_data[0];
        }

        return ptr;
    }

    byte _data[N + 1];
    byte* _head;
    byte* _tail;
};
