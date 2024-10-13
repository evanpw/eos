#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "estd/print.h"

static constexpr size_t BUFFER_SIZE = 4096;

char waitForKey(int tty) {
    char c;
    read(tty, &c, 1);
    return c;
}

int doLess(int fd, int tty) {
    char* buffer = new char[BUFFER_SIZE];

    bool screenDirty = false;  // we've written something since the last clear
    size_t currentRow = 0;     // 0-24
    size_t currentCol = 0;     // 0-79
    bool justWrapped = false;  // we wrapped to the next line after the previous character
    while (true) {
        ssize_t bytesRead = read(fd, buffer, BUFFER_SIZE);
        if (bytesRead < 0) {
            return errno;
        } else if (bytesRead == 0) {
            // EOF
            break;
        }

        for (size_t i = 0; i < bytesRead; ++i) {
            // If a newline immediately follows a line wrap, we can skip it
            if (justWrapped && buffer[i] == '\n') {
                justWrapped = false;
                continue;
            }

            putchar(buffer[i]);
            screenDirty = true;

            if (buffer[i] == '\n') {
                currentRow++;
                currentCol = 0;
            } else {
                currentCol++;
                if (currentCol == 80) {
                    currentRow++;
                    currentCol = 0;
                    justWrapped = true;
                }
            }

            // Once we reach the last line, we print a message and wait for a keypress
            // to continue
            if (currentRow == 24) {
                print("\033[30;47m-- More --\033[m");
                waitForKey(tty);

                // Clear the screen and continue with the next line
                print("\033[2J");
                currentRow = 0;
                currentCol = 0;
                screenDirty = false;
            }
        }
    }
    delete[] buffer;

    if (screenDirty) {
        print("\033[30;47m-- END --\033[m");
        waitForKey(tty);
    }

    return 0;
}

int main(int argc, char* argv[]) {
    int fd, tty;

    if (argc < 2) {
        // No filename given, read from stdin

        // If stdin is a terminal, then we don't have any input
        if (isatty(STDIN_FILENO)) {
            println("Usage: {} <file>", argv[0]);
            return 1;
        }

        fd = STDIN_FILENO;

        // If input comes from stdin, we need a different fd to get keyboard input
        tty = open("/dev/tty", 0);
        if (tty < 0) {
            println("{}: cannot open /dev/tty: {}", argv[0], errno);
            return 1;
        }
    } else {
        // Filename given on the command line, read from that

        fd = open(argv[1], 0);
        if (fd < 0) {
            println("{}: no such file or directory: {}", argv[0], argv[1]);
            return 1;
        }

        tty = STDIN_FILENO;
    }

    // Switch to alternate terminal mode
    print("\033[?1049h");

    int result = doLess(fd, tty);

    // Switch back to normal terminal mode
    print("\033[?1049l");

    if (result != 0) {
        println("{}: read error: {}", argv[0], result);
        return 1;
    }

    close(fd);
    return 0;
}
