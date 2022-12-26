#pragma once

#include <stdint.h>

typedef uint32_t Color; // RGBA

#define RED(x) ((x) >> 24)
#define GREEN(x) (((x) >> 16) & 0xff)
#define BLUE(x) (((x) >> 8) & 0xff)
#define ALPHA(x) ((x) & 0xff)

typedef struct {
	Color colorkey; // Used for image formats that do not support transparency.
	int count; // Length of colors[]
	Color colors[];
} Palette;

Palette *palette_create_default();
// This function returns NULL on failure.
Palette *palette_create_from_csv_file(char const *path);
// It is safe to free a NULL palette.
void palette_free(Palette *);
