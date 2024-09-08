#include <unistd.h>

#include "estd/print.h"

extern "C" int main() __attribute__((section(".entry")));

int main() {
    while (true) {
        sleep(20);
        print("u");
    }

    return 0;
}
