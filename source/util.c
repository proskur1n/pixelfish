#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <SDL2/SDL_error.h>
#include "util.h"

void *xmalloc(size_t size)
{
	void *p = malloc(size);
	if (!p) {
		fatal("No memory");
	}
	return p;
}

void fatal(char const *format, ...)
{
	va_list va;
	va_start(va, format);
	vfprintf(stderr, format, va);
	fprintf(stderr, "\n");
	va_end(va);
	exit(EXIT_FAILURE);
}

void fatalSDL(char const *format, ...)
{
	va_list va;
	va_start(va, format);
	vfprintf(stderr, format, va);
	va_end(va);

	char const *err = SDL_GetError();
	if (err == NULL || err[0] == '\0') {
		err = "Empty SDL_GetError message";
	}
	fprintf(stderr, ": %s\n", err);

	exit(EXIT_FAILURE);
}

void *memdup(void *src, size_t n)
{
	void *dest = xmalloc(n);
	memcpy(dest, src, n);
	return dest;
}
