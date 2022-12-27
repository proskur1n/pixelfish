#pragma once

#include <stddef.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define LENGTH(a) (sizeof(a) / sizeof(*a))

// Prints an error message and terminates the program.
#define fatal(...) def_fatal(__FILE__, __LINE__, __VA_ARGS__)

// Prints an error message including the SDL_GetError() information and terminates the program.
#define fatalSDL(...) def_fatalSDL(__FILE__, __LINE__, __VA_ARGS__)

// Terminates the program if it couldn't allocate enough memory. The allocated buffer will be
// filled with zeroes.
void *xalloc(size_t size) __attribute__ ((malloc));

// Should be called by the "fatal" macro.
void def_fatal(char const *file, int line, char const *format, ...) __attribute__ ((format (printf, 3, 4), noreturn));

// Should be called by the "fatalSDL" macro.
void def_fatalSDL(char const *file, int line, char const *format, ...) __attribute__ ((format (printf, 3, 4), noreturn));

// Creates a heap-allocated copy of data. Terminates the program if it couldn't allocate enough
// memory.
void *xmemdup(void *data, size_t n) __attribute__ ((malloc));
