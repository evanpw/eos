#include "estd/assertions.h"

#ifdef KERNEL
#include "panic.h"
#else
#include <stdlib.h>

#include "estd/print.h"
#endif

void __assertion_failed(const char* msg) {
#ifdef KERNEL
    panic(msg);
#else
    print("\nAssertion failed: ");
    print(msg);
    exit(EXIT_FAILURE);
#endif
}
