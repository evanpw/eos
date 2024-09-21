#include <unistd.h>

#include "estd/print.h"
#include "stdio.h"

int main() {
    for (char c = 'a'; c < 'd'; c++) {
        print("\033[s\033[0;80H");
        putchar(c);
        print("\033[u");
        sleep(20);
    }

    print("\033[s\033[0;80H \033[u");
    return 1;
}
