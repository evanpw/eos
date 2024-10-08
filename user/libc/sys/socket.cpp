#include <sys/socket.h>

#include "syscall.h"

int socket(int domain, int type, int protocol) {
    return try_syscall(SYS_socket, domain, type, protocol);
}

int connect(int fd, const struct sockaddr *address, socklen_t address_len) {
    return try_syscall(SYS_connect, fd, address, address_len);
}

ssize_t send(int socket, const void *buffer, size_t length, int flags) {
    return try_syscall(SYS_send, socket, buffer, length, flags);
}

ssize_t recv(int socket, void *buffer, size_t length, int flags) {
    return try_syscall(SYS_recv, socket, buffer, length, flags);
}

int listen(int socket, int backlog) { return try_syscall(SYS_listen, socket, backlog); }

int bind(int socket, const struct sockaddr *address, socklen_t address_len) {
    return try_syscall(SYS_bind, socket, address, address_len);
}

int accept(int socket, struct sockaddr *__restrict address,
           socklen_t *__restrict address_len) {
    return try_syscall(SYS_accept, socket, address, address_len);
}
