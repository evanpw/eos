#include <netinet/in.h>
#include <string.h>
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
        return 1;
    }

    // Send an HTTP request for the root file
    const char* request =
        "GET / HTTP/1.1\r\nHost: gh.evanpw.com\r\nConnection: close\r\n\r\n";
    ssize_t bytesSent = send(fd, request, strlen(request), 0);
    if (bytesSent < 0) {
        println("send failed");
        return 1;
    }

    // Echo the result to the terminal
    char* buffer = new char[1024];
    while (true) {
        ssize_t bytesRead = recv(fd, buffer, 1023, 0);
        if (bytesRead < 0) {
            println("tcp error");
            return 1;
        } else if (bytesRead == 0) {
            break;
        }

        buffer[bytesRead] = '\0';
        print(buffer);
    }
    delete[] buffer;

    close(fd);
    return 0;
}
