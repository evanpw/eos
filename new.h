#pragma once
#include <stddef.h>

// Placement new / delete
void *operator new(size_t, void *p) throw();
void *operator new[](size_t, void *p) throw();
void operator delete(void *, void *) throw();
void operator delete[](void *, void *) throw();

// Regular new / delete
void *operator new(size_t size);
void *operator new[](size_t size);
void operator delete(void *p);
void operator delete[](void *p);
void operator delete(void *p, size_t);
