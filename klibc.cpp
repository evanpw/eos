#include "klibc.h"  // IWYU pragma: keep

#include "mm.h"

void* malloc(size_t size) { return mm.kmalloc(size); }
void free(void* ptr) { return mm.kfree(ptr); }
