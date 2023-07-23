#include <unistd.h>

#include "estd/print.h"

extern "C" void main() __attribute__((section(".entry")));

void main() {
    while (true) {
        sleep(20);
        print("u");
    }
}
