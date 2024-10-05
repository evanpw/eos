#pragma once

#include <sys/socket.h>

#include "file.h"
#include "net/tcp.h"

class Socket : public File {
public:
    virtual int64_t connect(const struct sockaddr* addr, socklen_t addrlen) = 0;
    virtual bool isSocket() const override { return true; }
};

class TcpSocket : public Socket {
public:
    int64_t connect(const struct sockaddr* addr, socklen_t addrlen) override;
    ssize_t read(OpenFileDescription& fd, void* buffer, size_t count) override;
    ssize_t write(OpenFileDescription& fd, const void* buffer, size_t count) override;

private:
    TcpHandle _handle = InvalidTcpHandle;
};

class UdpSocket : public Socket {
public:
    int64_t connect(const struct sockaddr* addr, socklen_t addrlen) override;
    ssize_t read(OpenFileDescription& fd, void* buffer, size_t count) override;
    ssize_t write(OpenFileDescription& fd, const void* buffer, size_t count) override;
};
