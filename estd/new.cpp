#include "estd/new.h"

// Placement new / delete
void* operator new(size_t, void* ptr) { return ptr; }
void* operator new[](size_t, void* ptr) { return ptr; }
void operator delete(void*, void*) noexcept {}
void operator delete[](void*, void*) noexcept {}

// TODO: new C++17 aligned new / delete

// Regular (non-placement) new / delete
#ifdef KERNEL
#include "mm.h"
void* operator new(size_t count) { return mm.kmalloc(count); }
void* operator new[](size_t count) { return mm.kmalloc(count); }
void operator delete(void* ptr) noexcept { mm.kfree(ptr); }
void operator delete[](void* ptr) noexcept { mm.kfree(ptr); }
void operator delete(void* ptr, size_t) noexcept { mm.kfree(ptr); }
void operator delete[](void* ptr, size_t) noexcept { mm.kfree(ptr); }
#else
#include "stdlib.h"
void* operator new(size_t count) { return malloc(count); }
void* operator new[](size_t count) { return malloc(count); }
void operator delete(void* ptr) noexcept { free(ptr); }
void operator delete[](void* ptr) noexcept { free(ptr); }
void operator delete(void* ptr, size_t) noexcept { free(ptr); }
void operator delete[](void* ptr, size_t) noexcept { free(ptr); }
#endif
