#include <stdio.h>
#include <unistd.h>

#include "estd/print.h"

int main(int argc, char* argv[]) {
    println("stdin->fd: {}", fileno(stdin));
    println("stdout->fd: {}", fileno(stdout));
    println("stderr->fd: {}", fileno(stderr));

    char buffer[64];
    int fd = open("/etc/longfile.txt", 0);
    lseek(fd, 144, SEEK_SET);
    size_t bytesRead = read(fd, buffer, 64);
    buffer[bytesRead] = '\0';
    fputs(buffer, stdout);

    return 0;
}
