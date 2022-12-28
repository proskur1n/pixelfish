#pragma once

#include <stdint.h>

typedef uint32_t Color; // RGBA

#define RED(x) ((x) >> 24)
#define GREEN(x) (((x) >> 16) & 0xff)
#define BLUE(x) (((x) >> 8) & 0xff)
#define ALPHA(x) ((x) & 0xff)

// Converts SDL_Color to Color
#define COLOR_FROM(c) ((c.r << 24) | (c.g << 16) | (c.b << 8) | c.a)

typedef struct {
	Color colorkey; // Used for image formats that do not support transparency.
	int count; // Length of colors[]
	Color const *colors;
} Palette;

// Returns the default palette. Note that it must be freed with palette_free and cannot be passed
// to a normal free().
Palette palette_get_default();

// TODO: It does not return NULL with the current specification.
// This function returns NULL on failure.
Palette palette_create_from_csv_file(char const *path);

void palette_free(Palette palette);
