#include "estd/new.h"

// Placement new / delete
void* operator new(size_t, void* ptr) { return ptr; }
void* operator new[](size_t, void* ptr) { return ptr; }
void operator delete(void*, void*) noexcept {}
void operator delete[](void*, void*) noexcept {}

// Regular (non-placement) new / delete
#ifdef KERNEL
#include "system.h"
void* operator new(size_t count) { return System::mm().kmalloc(count); }
void* operator new[](size_t count) { return System::mm().kmalloc(count); }
void operator delete(void* ptr) { System::mm().kfree(ptr); }
void operator delete[](void* ptr) { System::mm().kfree(ptr); }
void operator delete(void* ptr, size_t) { System::mm().kfree(ptr); }
void operator delete[](void* ptr, size_t) { System::mm().kfree(ptr); }
#else
#include "estd/assertions.h"
void* operator new(size_t) { ASSERT(false); }
void* operator new[](size_t) { ASSERT(false); }
void operator delete(void*) { ASSERT(false); }
void operator delete[](void*) { ASSERT(false); }
void operator delete(void*, size_t) { ASSERT(false); }
void operator delete[](void*, size_t) { ASSERT(false); }
#endif
