#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include "estd/print.h"

int main(int argc, char* argv[]) {
    // Save the existing terminal settings
    termios oldSettings;
    tcgetattr(STDIN_FILENO, &oldSettings);

    // Switch to raw mode
    termios newSettings = oldSettings;
    cfmakeraw(&newSettings);
    newSettings.c_oflag |= OPOST;
    tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);

    char buffer[64];
    while (true) {
        ssize_t bytesRead = read(STDIN_FILENO, buffer, 64);
        if (bytesRead <= 0) {
            break;
        }

        // Ctrl-D
        if (buffer[0] == '\x04') {
            break;
        }

        print("input: ");
        for (size_t i = 0; i < bytesRead; i++) {
            char c = buffer[i];
            if (c >= 0x20 && c <= 0x7E) {
                putchar(c);
            } else {
                putchar('^');
                putchar(c ^ 0x40);
            }
        }
        println("");
    }

    // Switch back to regular terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);

    return 0;
}
