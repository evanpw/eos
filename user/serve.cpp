#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "estd/print.h"

int main(int argc, char* argv[]) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        println("socket failed");
        return 1;
    }

    println("got socket: {}", fd);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (sockaddr*)&addr, sizeof(sockaddr_in)) < 0) {
        println("bind failed");
        return 1;
    }

    println("bound");

    if (listen(fd, 2) < 0) {
        println("listen failed");
        return 1;
    }

    println("listened");

    int client_fd = accept(fd, nullptr, nullptr);
    if (client_fd < 0) {
        println("accept failed");
        return 1;
    }

    println("accepted: {}", client_fd);

    /*
    char buf[1024];
    int n = read(client_fd, buf, sizeof(buf));
    if (n < 0) {
        println("read failed");
        return 1;
    }

    buf[n] = 0;
    println("read: {}", buf);

    const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nHello";
    n = write(client_fd, response, strlen(response));
    if (n < 0) {
        println("write failed");
        return 1;
    }

    println("wrote: {}", n);
    */

    sleep(500);
    close(client_fd);
    println("closed: {}", client_fd);

    // sleep(500);
    println("closing");
    close(fd);
    println("closed");
    return 0;
}
