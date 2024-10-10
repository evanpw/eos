#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "estd/print.h"

int main(int argc, char* argv[]) {
    int fds[2];

    int result = pipe(fds);
    if (result < 0) {
        println("{}: error creating pipe: {}", argv[0], errno);
        return 1;
    }

    const char* message = "Hello, world!";
    ssize_t bytesWritten = write(fds[1], message, strlen(message));
    if (bytesWritten < 0) {
        println("{}: error writing to pipe: {}", argv[0], errno);
        return 1;
    }

    char buffer[4096];
    ssize_t bytesRead = read(fds[0], buffer, 4095);
    if (bytesRead < 0) {
        println("{}: error reading from pipe: {}", argv[0], errno);
        return 1;
    }

    buffer[bytesRead] = '\0';

    println("message from pipe: {}", buffer);

    close(fds[0]);
    close(fds[1]);
    return 0;
}
