#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "estd/print.h"
#include "estd/string.h"
#include "estd/vector.h"

void appendString(estd::vector<char>& v, const char* str) {
    for (size_t i = 0; i < strlen(str); i++) {
        v.push_back(str[i]);
    }
}

int main(int argc, char* argv[]) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

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

    // Send an HTTP request for the root file]
    estd::string request = "GET /";
    if (argc > 1) {
        request += argv[1];
    }
    request += " HTTP/1.1\r\nHost: gh.evanpw.com\r\nConnection: close\r\n\r\n";

    ssize_t bytesSent = send(fd, request.c_str(), request.size(), 0);
    if (bytesSent < 0) {
        println("send failed");
        return 1;
    }

    // Echo the result to the terminal
    char* buffer = new char[64 * 1024];
    while (true) {
        ssize_t bytesRead = recv(fd, buffer, 64 * 1024, 0);
        if (bytesRead < 0) {
            println("tcp error");
            return 1;
        } else if (bytesRead == 0) {
            break;
        }

        write(STDOUT_FILENO, buffer, bytesRead);
    }

    close(fd);
    return 0;
}
