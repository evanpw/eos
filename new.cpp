#include "new.h"

#include "system.h"

void *operator new(size_t, void *p) throw() { return p; }
void *operator new[](size_t, void *p) throw() { return p; }
void operator delete(void *, void *) throw(){};
void operator delete[](void *, void *) throw(){};
void *operator new(size_t size) { return System::mm().kmalloc(size); }
void *operator new[](size_t size) { return System::mm().kmalloc(size); }
void operator delete(void *p) { return System::mm().kfree(p); }
void operator delete[](void *p) { return System::mm().kfree(p); }
