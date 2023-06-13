#include "new.h"

#include "system.h"

// Placement new / delete
void *operator new(size_t, void *p) throw() { return p; }
void *operator new[](size_t, void *p) throw() { return p; }
void operator delete(void *, void *) throw(){};
void operator delete[](void *, void *) throw(){};

// Regular new / delete
void *operator new(size_t size) { return System::mm().kmalloc(size); }
void *operator new[](size_t size) { return System::mm().kmalloc(size); }
void operator delete(void *p) { System::mm().kfree(p); }
void operator delete[](void *p) { System::mm().kfree(p); }
void operator delete(void *p, size_t) { System::mm().kfree(p); }
