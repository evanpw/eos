#include <unistd.h>

#include "estd/print.h"
#include "stdio.h"

extern "C" int main() __attribute__((section(".entry")));

int main() {
    char c = 'a';
    while (true) {
        print("\033[s\033[0;80H");
        putchar(c);
        print("\033[u");
        sleep(20);

        c = (c == 'z') ? 'a' : c + 1;
    }

    return 0;
}
