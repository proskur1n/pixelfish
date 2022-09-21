#pragma once

#include <stddef.h>

#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define LENGTH(a) (sizeof(a) / sizeof(*a))

// Terminates the program if it couldn't allocate enough memory.
void *xmalloc(size_t size) __attribute__ ((malloc));

// Prints an error message and terminates the program.
void fatal(char const *format, ...) __attribute__ ((format (printf, 1, 2), noreturn));

// Prints an error message including the SDL_GetError() information and
// terminates the program.
void fatalSDL(char const *format, ...) __attribute__ ((format (printf, 1, 2), noreturn));

