#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include "estd/print.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        // TODO: print to stderr
        println("Usage: {} <file>", argv[0]);
        return 1;
    }

    int fd = open(argv[1], 0);
    if (fd < 0) {
        println("{}: no such file or directory: {}", argv[0], argv[1]);
        return 1;
    }

    char buffer[4096];
    ssize_t bytesRead;
    while ((bytesRead = read(fd, buffer, 4095)) > 0) {
        buffer[bytesRead] = '\0';
        print("{}", buffer);
    }

    if (bytesRead < 0) {
        println("{}: error reading file", argv[0]);
        return 1;
    }

    close(fd);
    return 0;
}
