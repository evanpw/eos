#include "socket.h"

#include <netinet/in.h>

#include "klibc.h"
#include "net/ip.h"

TcpSocket::TcpSocket() { _handle = tcpOpen(); }

TcpSocket::TcpSocket(TcpHandle handle) : _handle(handle) {}

TcpSocket::~TcpSocket() { tcpClose(_handle); }

int64_t TcpSocket::bind(const struct sockaddr* addr, socklen_t addrlen) {
    if (_handle == InvalidTcpHandle) {
        return -EINVAL;
    }

    if (addr->sa_family != AF_INET) {
        return -EAFNOSUPPORT;
    }

    if (addrlen != sizeof(struct sockaddr_in)) {
        return -EINVAL;
    }

    const struct sockaddr_in* addr_in = reinterpret_cast<const struct sockaddr_in*>(addr);
    IpAddress sourceIp(addr_in->sin_addr.s_addr);
    uint16_t sourcePort = ntohs(addr_in->sin_port);

    if (!tcpBind(_handle, sourceIp, sourcePort)) {
        return -EADDRINUSE;
    }

    return 0;
}

int64_t TcpSocket::listen(int backlog) {
    if (backlog < 0) {
        return -EINVAL;
    }

    if (!tcpListen(_handle, backlog)) {
        return -EADDRINUSE;
    }

    return 0;
}

estd::shared_ptr<Socket> TcpSocket::accept(sockaddr* addr, socklen_t* addrlen) {
    if (addr && !addrlen) {
        // EINVAL
        return {};
    }

    TcpHandle childHandle = tcpAccept(_handle);
    if (childHandle == InvalidTcpHandle) {
        // ECONNABORTED or EINVAL
        return {};
    }

    if (addr) {
        struct sockaddr_in* addr_in = reinterpret_cast<struct sockaddr_in*>(addr);
        addr_in->sin_family = AF_INET;
        addr_in->sin_addr.s_addr = tcpRemoteIp(childHandle);
        addr_in->sin_port = htons(tcpRemotePort(childHandle));

        memcpy(addr, addr_in, min<size_t>(*addrlen, sizeof(struct sockaddr_in)));
        *addrlen = sizeof(struct sockaddr_in);
    }

    return estd::shared_ptr<TcpSocket>(new TcpSocket(childHandle));
}

int64_t TcpSocket::connect(const struct sockaddr* addr, socklen_t addrlen) {
    if (addr->sa_family != AF_INET) {
        return -EAFNOSUPPORT;
    }

    if (addrlen != sizeof(struct sockaddr_in)) {
        return -EINVAL;
    }

    const struct sockaddr_in* addr_in = reinterpret_cast<const struct sockaddr_in*>(addr);
    IpAddress destIp(addr_in->sin_addr.s_addr);
    uint16_t destPort = ntohs(addr_in->sin_port);

    if (!tcpConnect(_handle, destIp, destPort)) {
        return -ECONNREFUSED;
    }

    // Block until the handshake finishes
    if (!tcpWaitForConnection(_handle)) {
        return -ECONNREFUSED;
    }

    return 0;
}

ssize_t TcpSocket::read(OpenFileDescription&, void* buffer, size_t size) {
    return tcpRecv(_handle, buffer, size);
}

ssize_t TcpSocket::write(OpenFileDescription&, const void* buffer, size_t size) {
    if (!tcpSend(_handle, buffer, size, true)) {
        return -EPIPE;
    }

    return size;
}

int64_t UdpSocket::bind(const struct sockaddr*, socklen_t) { return -ENOSYS; }
int64_t UdpSocket::listen(int) { return -ENOSYS; }
estd::shared_ptr<Socket> UdpSocket::accept(sockaddr*, socklen_t*) { return {}; }
int64_t UdpSocket::connect(const struct sockaddr*, socklen_t) { return -ENOSYS; }
ssize_t UdpSocket::read(OpenFileDescription&, void*, size_t) { return -ENOSYS; }
ssize_t UdpSocket::write(OpenFileDescription&, const void*, size_t) { return -ENOSYS; }
