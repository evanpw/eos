#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

extern "C" void umain() __attribute__((section(".entry")));

void umain() {
    const char* msg = "Hello World!\n";
    write(1, msg, 13);

    if (getpid() == 1) {
        const char* msg2 = "pid=1\n";
        write(1, msg2, 6);
    }

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
