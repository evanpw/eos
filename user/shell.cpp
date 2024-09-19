#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "estd/print.h"

int main() {
    char buffer[64];

    int fd = open("/etc/version.txt", 0);
    ssize_t bytesRead = read(fd, buffer, 63);
    buffer[bytesRead] = '\0';
    print("\033[31;42m{}\033[m", buffer);
    close(fd);

    println("pid={}", getpid());

    while (true) {
        print("$ ");

        bytesRead = read(STDIN_FILENO, buffer, 63);
        if (bytesRead == 0) {
            println("EOF");
            continue;
        }

        // Trim trailing newline and/or add null terminator
        if (bytesRead > 0 && buffer[bytesRead - 1] == '\n') {
            buffer[bytesRead - 1] = '\0';
        } else {
            buffer[bytesRead] = '\0';
        }

        if (strcmp(buffer, "clear") == 0) {
            print("\033[J");
        } else if (strcmp(buffer, "spam") == 0) {
            launch("/bin/spam.bin");
        } else {
            println("no such command: {}", buffer);
        }
    }

    return 0;
}
