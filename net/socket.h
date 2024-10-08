#pragma once

#include <sys/socket.h>

#include "estd/shared_ptr.h"
#include "file.h"
#include "net/tcp.h"

class Socket : public File {
public:
    virtual int64_t bind(const struct sockaddr* addr, socklen_t addrlen) = 0;
    virtual int64_t listen(int backlog) = 0;
    virtual estd::shared_ptr<Socket> accept(sockaddr* addr, socklen_t* addrlen) = 0;
    virtual int64_t connect(const struct sockaddr* addr, socklen_t addrlen) = 0;
    virtual bool isSocket() const override { return true; }
};

class TcpSocket : public Socket {
public:
    TcpSocket();
    TcpSocket(TcpHandle handle);
    ~TcpSocket();

    int64_t bind(const struct sockaddr* addr, socklen_t addrlen) override;
    int64_t listen(int backlog) override;
    estd::shared_ptr<Socket> accept(sockaddr* addr, socklen_t* addrlen) override;
    int64_t connect(const struct sockaddr* addr, socklen_t addrlen) override;
    ssize_t read(OpenFileDescription& fd, void* buffer, size_t count) override;
    ssize_t write(OpenFileDescription& fd, const void* buffer, size_t count) override;

private:
    TcpHandle _handle;
};

class UdpSocket : public Socket {
public:
    int64_t bind(const struct sockaddr* addr, socklen_t addrlen) override;
    int64_t listen(int backlog) override;
    estd::shared_ptr<Socket> accept(sockaddr* addr, socklen_t* addrlen) override;
    int64_t connect(const struct sockaddr* addr, socklen_t addrlen) override;
    ssize_t read(OpenFileDescription& fd, void* buffer, size_t count) override;
    ssize_t write(OpenFileDescription& fd, const void* buffer, size_t count) override;
};
