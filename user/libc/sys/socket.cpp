#include <sys/socket.h>

#include "syscall.h"

int socket(int domain, int type, int protocol) {
    return try_syscall(SYS_socket, domain, type, protocol);
}

int connect(int fd, const struct sockaddr *address, socklen_t address_len) {
    return try_syscall(SYS_connect, fd, address, address_len);
}
