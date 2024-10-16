#include <termios.h>
#include <unistd.h>

#include "estd/print.h"

int main(int argc, char* argv[]) {
    struct termios termSettings;
    println("1");
    tcgetattr(STDIN_FILENO, &termSettings);
    println("2");
    cfmakeraw(&termSettings);
    termSettings.c_oflag |= OPOST;
    println("3");
    tcsetattr(STDIN_FILENO, TCSANOW, &termSettings);
    println("4");

    char buffer[64];
    while (true) {
        ssize_t bytesRead = read(STDIN_FILENO, buffer, 63);
        if (bytesRead <= 0) {
            break;
        }

        buffer[bytesRead] = '\0';
        println("input: <{}>", buffer);
    }

    return 0;
}
