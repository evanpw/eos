#pragma once

#include <stdint.h>
#include <sys/uio.h>

typedef uint32_t socklen_t;
typedef uint16_t sa_family_t;

struct sockaddr {
    sa_family_t sa_family;
    char sa_data[];
};

struct sockaddr_storage {
    sa_family_t ss_family;
    char ss_data[128 - sizeof(sa_family_t) - sizeof(unsigned long)];

    // This forces the alignment of sockaddr_storage to be 8 bytes.
    unsigned long __ss_align;
};

struct msghdr {
    void *msg_name;
    socklen_t msg_namelen;
    struct iovec *msg_iov;
    int msg_iovlen;
    void *msg_control;
    socklen_t msg_controllen;
    int msg_flags;
};

struct cmsghdr {
    socklen_t cmsg_len;
    int cmsg_level;
    int cmsg_type;
};

#define SCM_RIGHTS 1

#define CMSG_DATA(cmsg) ((unsigned char *)(cmsg) + sizeof(cmsghdr))
#define CMSG_NXTHDR(mhdr, cmsg)                                         \
    ((char *)(cmsg) + (cmsg)->cmsg_len - (char *)(mhdr)->msg_control >= \
             (mhdr)->msg_controllen                                     \
         ? NULL                                                         \
         : (cmsghdr *)((char *)(cmsg) + (cmsg)->cmsg_len))
#define CMSG_FIRSTHDR(mhdr) \
    ((mhdr)->msg_controllen >= sizeof(cmsghdr) ? (cmsghdr *)(mhdr)->msg_control : NULL)

struct linger {
    int l_onoff;
    int l_linger;
};

#define SOCK_DGRAM 1
#define SOCK_RAW 2
#define SOCK_SEQPACKET 3
#define SOCK_STREAM 4

// For the level argument of setsockopt and getsockopt
#define SOL_SOCKET 1

// For the optname argument of setsockopt and getsockopt
#define SO_ACCEPTCONN 1
#define SO_BROADCAST 2
#define SO_DEBUG 3
#define SO_DONTROUTE 4
#define SO_ERROR 5
#define SO_KEEPALIVE 6
#define SO_LINGER 7
#define SO_OOBINLINE 8
#define SO_RCVBUF 9
#define SO_RCVLOWAT 10
#define SO_RCVTIMEO 11
#define SO_REUSEADDR 12
#define SO_SNDBUF 13
#define SO_SNDLOWAT 14
#define SO_SNDTIMEO 15
#define SO_TYPE 16

// Maximum backlog queue length which can be passed to listen
#define SOMAXCONN 128

// For the msg_flags field in msghdr or the flags parameter in recfrom, recvmsg, etc.
#define MSG_CTRUNC 1
#define MSG_DONTROUTE 2
#define MSG_EOR 3
#define MSG_OOB 4
#define MSG_PEEK 5
#define MSG_TRUNC 6
#define MSG_WAITALL 7

#define AF_INET 1
#define AF_INET6 2
#define AF_UNIX 3
#define AF_UNSPEC 4

#define SHUT_RD 1
#define SHUT_RDWR 2
#define SHUT_WR 3

int accept(int, struct sockaddr *__restrict, socklen_t *__restrict);
int bind(int, const struct sockaddr *, socklen_t);
int connect(int, const struct sockaddr *, socklen_t);
int getpeername(int, struct sockaddr *__restrict, socklen_t *__restrict);
int getsockname(int, struct sockaddr *__restrict, socklen_t *__restrict);
int getsockopt(int, int, int, void *__restrict, socklen_t *__restrict);
int listen(int, int);
ssize_t recv(int, void *, size_t, int);
ssize_t recvfrom(int, void *__restrict, size_t, int, struct sockaddr *__restrict,
                 socklen_t *__restrict);
ssize_t recvmsg(int, struct msghdr *, int);
ssize_t send(int, const void *, size_t, int);
ssize_t sendmsg(int, const struct msghdr *, int);
ssize_t sendto(int, const void *, size_t, int, const struct sockaddr *, socklen_t);
int setsockopt(int, int, int, const void *, socklen_t);
int shutdown(int, int);
int socket(int, int, int);
int sockatmark(int);
int socketpair(int, int, int, int[2]);
