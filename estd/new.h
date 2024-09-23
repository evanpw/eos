// IWYU pragma: always_keep
#pragma once
#include <stddef.h>

// Placement new / delete
void* operator new(size_t count, void* ptr);
void* operator new[](size_t count, void* ptr);
void operator delete(void* ptr, void* place) noexcept;
void operator delete[](void* ptr, void* place) noexcept;
