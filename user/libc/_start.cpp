#include <unistd.h>

extern "C" void _start() __attribute__((section(".entry")));
extern "C" int main();

void _start() {
    int result = main();
    _exit(result);
}
