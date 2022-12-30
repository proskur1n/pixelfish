#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <SDL2/SDL_error.h>
#include "util.h"

void *xalloc(size_t size)
{
	char *p = calloc(size, sizeof(char));
	if (!p) {
		fatal("No memory");
	}
	return p;
}

void def_fatal(char const *file, int line, char const *format, ...)
{
	va_list va;
	va_start(va, format);
	fprintf(stderr, "[pixelfish] %s:%d\n\t", file, line);
	vfprintf(stderr, format, va);
	fprintf(stderr, "\n");
	va_end(va);
	exit(EXIT_FAILURE);
}

void def_fatalSDL(char const *file, int line, char const *format, ...)
{
	va_list va;
	va_start(va, format);
	fprintf(stderr, "[pixelfish] %s:%d\n\t", file, line);
	vfprintf(stderr, format, va);
	va_end(va);

	char const *err = SDL_GetError();
	if (err == NULL || err[0] == '\0') {
		err = "Empty SDL_GetError message";
	}
	fprintf(stderr, ": %s\n", err);

	exit(EXIT_FAILURE);
}

void *xmemdup(void const *src, size_t n)
{
	void *dest = malloc(n);
	if (!dest) {
		fatal("No memory");
	}
	return memcpy(dest, src, n);
}

char *xstrdup(char const *str) {
	char *copy = strdup(str);
	if (!copy) {
		fatal("No memory");
	}
	return copy;
}
