#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "estd/print.h"

int main(int argc, char* argv[]) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    println("socket: {}", fd);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    // gh.evanpw.com = 185.199.110.153
    addr.sin_addr.s_addr =
        ((uint32_t)153 << 24) | ((uint32_t)110 << 16) | ((uint32_t)199 << 8) | 185;

    if (connect(fd, (sockaddr*)&addr, sizeof(sockaddr_in)) < 0) {
        println("connect failed");
        return -1;
    }

    sleep(100);
    close(fd);
    return 0;
}
