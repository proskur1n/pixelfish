#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "palette.h"
#include "util.h"

Palette *palette_create_default()
{
	// TODO: Choose default palette
	Color colors[] = {
		0xf1fbfeff, 0xa8f7feff, 0x84e0abff,
		0x8fb569ff, 0xa7ab59ff, 0x99943cff,
		0x7a6f18ff, 0x7a6507ff
	};
	Palette *p = xalloc(sizeof(Palette) + sizeof(Color) * LENGTH(colors));
	p->colorkey = 0;
	p->count = LENGTH(colors);
	memcpy(p->colors, colors, sizeof(colors));
	return p;
}

Palette *palette_create_from_csv_file(char const *path)
{
	// TODO
	assert(0 && "Not implemented!");
	return NULL;
}

void palette_free(Palette *p)
{
	free(p);
}
