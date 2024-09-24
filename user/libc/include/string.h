#pragma once
#include <stddef.h>

extern "C" {
void* memccpy(void* __restrict dest, const void* __restrict src, int c, size_t n);
void* memchr(const void* s, int c, size_t n);
int memcmp(const void* s1, const void* s2, size_t n);
void* memcpy(void* __restrict dest, const void* __restrict src, size_t n);
void* memmove(void* dest, const void* src, size_t n);
void* memset(void* s, int c, size_t n);
char* strcat(char* __restrict dest, const char* __restrict src);
const char* strchr(const char* s, int c);
int strcmp(const char* s1, const char* s2);
int strcoll(const char* s1, const char* s2);
char* strcpy(char* __restrict dest, const char* __restrict src);
size_t strcspn(const char* s, const char* reject);
char* strdup(const char* s);
char* strerror(int errnum);
int* strerror_r(int errnum, char* buf, size_t buflen);
size_t strlen(const char* s);
char* strncat(char* __restrict dest, const char* __restrict src, size_t n);
int strncmp(const char* s1, const char* s2, size_t n);
char* strncpy(char* __restrict dest, const char* __restrict src, size_t n);
char* strpbrk(const char* s, const char* accept);
char* strrchr(const char* s, int c);
size_t strspn(const char* s, const char* accept);
char* strstr(const char* haystack, const char* needle);
char* strtok(char* __restrict str, const char* __restrict delim);
char* strtok_r(char* str, const char* delim, char** saveptr);
size_t strxfrm(char* __restrict dest, const char* __restrict src, size_t n);
}
