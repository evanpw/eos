#include <fcntl.h>
#include <unistd.h>

#include "estd/print.h"

extern "C" void main() __attribute__((section(".entry")));

void main() {
    char buffer[64];

    int fd = open("version.txt", 0);
    ssize_t bytesRead = read(fd, buffer, 64);
    write(1, buffer, bytesRead);

    println("pid={}", getpid());

    while (true) {
        while (read(0, buffer, 64) == 0) {
        }

        if (buffer[0] == 'y') {
            write(1, "\byes\n", 5);
        } else {
            write(1, "\bno\n", 4);
        }
    }
}
