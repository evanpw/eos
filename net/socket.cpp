#include "socket.h"

#include <netinet/in.h>

#include "net/ip.h"

TcpSocket::~TcpSocket() { tcpClose(_handle); }

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

    _handle = tcpConnect(destIp, destPort);

    if (_handle == InvalidTcpHandle) {
        return -ECONNREFUSED;
    }

    // Block until the handshake finishes
    if (!tcpWaitForConnection(_handle)) {
        return -ECONNREFUSED;
    }

    return 0;
}

ssize_t TcpSocket::read(OpenFileDescription&, void* buffer, size_t size) {
    if (_handle == InvalidTcpHandle) {
        return -ENOTCONN;
    }

    return tcpRecv(_handle, buffer, size);
}

ssize_t TcpSocket::write(OpenFileDescription&, const void* buffer, size_t size) {
    if (_handle == InvalidTcpHandle) {
        return -ENOTCONN;
    }

    if (!tcpSend(_handle, buffer, size, true)) {
        return -EPIPE;
    }

    return size;
}

int64_t UdpSocket::connect(const struct sockaddr* /*addr*/, socklen_t /*addrlen*/) {
    return -ENOSYS;
}

ssize_t UdpSocket::read(OpenFileDescription&, void*, size_t) { return -ENOSYS; }
ssize_t UdpSocket::write(OpenFileDescription&, const void*, size_t) { return -ENOSYS; }
