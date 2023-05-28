#pragma once
#include "mem.h"

inline void *operator new(size_t, void *p) throw() { return p; }
inline void *operator new[](size_t, void *p) throw() { return p; }
inline void operator delete(void *, void *) throw(){};
inline void operator delete[](void *, void *) throw(){};

inline void* operator new(size_t size) {
    return MM.kmalloc(size);
}

inline void* operator new[](size_t size) {
    return MM.kmalloc(size);
}

inline void operator delete(void* p) {
    return MM.kfree(p);
}

inline void operator delete[](void* p) {
    return MM.kfree(p);
}
