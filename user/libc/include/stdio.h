#pragma once

#include <stddef.h>

#include "bits/stdio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _IO_FILE FILE;

#define stdin (&_IO_stdin)
#define stdout (&_IO_stdout)
#define stderr (&_IO_stderr)

#define EOF -1
#define BUFSIZ 4096

int fileno(FILE* stream);
int feof(FILE* stream);
int ferror(FILE* stream);
void clearerr(FILE* stream);

int fclose(FILE* stream);

int fgetc(FILE* stream);
int getc(FILE* stream);
int getchar();

int fputc(int c, FILE* stream);
int putc(int c, FILE* stream);
int putchar(int c);

int fputs(const char* s, FILE* stream);
int puts(const char* s);

size_t fread(void* ptr, size_t size, size_t nitems, FILE* stream);
size_t fwrite(const void* ptr, size_t size, size_t nitems, FILE* stream);

int setvbuf(FILE* stream, char* buf, int type, size_t size);
void setbuf(FILE* stream, char* buf);

int fflush(FILE* stream);

#ifdef __cplusplus
}
#endif
