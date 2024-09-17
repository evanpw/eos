#include <fcntl.h>
#include <unistd.h>

#include "estd/print.h"

int main() {
    char buffer[64];

    int fd = open("/etc/version.txt", 0);
    ssize_t bytesRead = read(fd, buffer, 64);
    write(STDOUT_FILENO, "\033[31;42m", 8);
    write(STDOUT_FILENO, buffer, bytesRead);
    write(STDOUT_FILENO, "\033[m", 3);
    close(fd);

    println("pid={}", getpid());

    while (true) {
        bytesRead = read(STDIN_FILENO, buffer, 1);
        if (bytesRead == 0) {
            println("EOF");
            continue;
        }

        if (buffer[0] == 'c') {
            write(1, "\033[J", 3);
        } else if (buffer[0] == 's') {
            launch("/bin/spam.bin");
        } else if (buffer[0] == 'y') {
            write(1, "\byes\n", 5);
        } else {
            write(1, "\bno\n", 4);
        }
    }

    return 0;
}
