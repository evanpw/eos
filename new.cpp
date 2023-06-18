#include "system.h"

// Regular (non-placement) new / delete
void *operator new(size_t size) { return System::mm().kmalloc(size); }
void *operator new[](size_t size) { return System::mm().kmalloc(size); }
void operator delete(void *p) { System::mm().kfree(p); }
void operator delete[](void *p) { System::mm().kfree(p); }
void operator delete(void *p, size_t) { System::mm().kfree(p); }
