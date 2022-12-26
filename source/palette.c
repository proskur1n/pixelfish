#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "palette.h"
#include "util.h"

static Color default_colors[] = {
	0xf1fbfeff,
	0xa8f7feff,
	0x84e0abff,
	0x8fb569ff,
	0xa7ab59ff,
	0x99943cff,
	0x7a6f18ff,
	0x7a6507ff
};

Palette palette_get_default()
{
	return (Palette) {
		.colorkey = 0x00,
		.count = LENGTH(default_colors),
		.colors = default_colors
	};
}

Palette palette_create_from_csv_file(char const *path)
{
	// TODO
	assert(0 && "Not implemented!");
	// return NULL;
	return (Palette) {0};
}

void palette_free(Palette *p)
{
	if (p != NULL && p->colors != default_colors) {
		free(p->colors);
	}
}
