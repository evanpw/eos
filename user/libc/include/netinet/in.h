#pragma once
#include <arpa/inet.h>
#include <stdint.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

struct in_addr {
    in_addr_t s_addr;
};

struct sockaddr_in {
    sa_family_t sin_family;   // AF_INET
    in_port_t sin_port;       // port number (network byte order)
    struct in_addr sin_addr;  // ip address (network byte order)
};

// For use as values of the level argument of getsocketopt and setsockopt
#define IPPROTO_IP 1
#define IPPROTO_ICMP 2
#define IPPROTO_TCP 3
#define IPPROTO_UDP 4

// Special ip addresses
#define INADDR_ANY 0                 // (0.0.0.0)
#define INADDR_BROADCAST 0xFFFFFFFF  // (255.255.255.255)

// Length of an ip address string
#define INET_ADDRSTRLEN 16

#ifdef __cplusplus
}
#endif
