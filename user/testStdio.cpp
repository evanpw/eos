#include <stdio.h>

#include "estd/print.h"

int main(int argc, char* argv[]) {
    println("stdin->fd: {}", fileno(stdin));
    println("stdout->fd: {}", fileno(stdout));
    println("stderr->fd: {}", fileno(stderr));

    return 0;
}
