#include <unistd.h>

#include "estd/print.h"
#include "stdio.h"

extern "C" int main() __attribute__((section(".entry")));

int main() {
    for (char c = 'a'; c < 'j'; c++) {
        print("\033[s\033[0;80H");
        putchar(c);
        print("\033[u");
        sleep(20);
    }

    // TODO: allow just returning
    _exit(12);

    return 0;
}
