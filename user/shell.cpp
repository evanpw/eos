#include <unistd.h>

#include "estd/print.h"

extern "C" void main() __attribute__((section(".entry")));

void main() {
    const char* msg = "Hello World!\n";
    write(1, msg, 13);

    println("pid={}", getpid());

    char buffer[64];
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
