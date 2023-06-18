#include "estd/new.h"

// Placement new / delete
void* operator new(size_t count, void* ptr) { return ptr; }
void* operator new[](size_t count, void* ptr) { return ptr; }
void operator delete(void* ptr, void* place) noexcept {}
void operator delete[](void* ptr, void* place) noexcept {}

// Regular (non-placement) new / delete
#ifdef KERNEL
#include "system.h"
void* operator new(size_t count) { return System::mm().kmalloc(count); }
void* operator new[](size_t count) { return System::mm().kmalloc(count); }
void operator delete(void* ptr) { System::mm().kfree(ptr); }
void operator delete[](void* ptr) { System::mm().kfree(ptr); }
void operator delete(void* ptr, size_t count) { System::mm().kfree(ptr); }
#else
#include "estd/assertions.h"
void* operator new(size_t count) { ASSERT(false); }
void* operator new[](size_t count) { ASSERT(false); }
void operator delete(void* ptr) { ASSERT(false); }
void operator delete[](void* ptr) { ASSERT(false); }
void operator delete(void* ptr, size_t count) { ASSERT(false); }
#endif
