#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "brush.h"
#include "util.h"

static int const MIN_STENCIL_BYTES = 12 * 12;

// The stencil buffer must have at least size * size elements.
static void fill_stencil(uint8_t *stencil, int size, bool round)
{
	if (round) {
		if (size == 3) {
			// Special case
			uint8_t s[9] = {0, 0xff, 0, 0xff, 0xff, 0xff, 0, 0xff, 0};
			memcpy(stencil, s, 9);
			return;
		}

		memset(stencil, 0, size * size);
		float half = ((float) size) / 2.0f;
		float cx = half - 0.5f;
		float cy = half - 0.5f;
		int i = 0;
		for (int y = 0; y < size; ++y) {
			for (int x = 0; x < size; ++x) {
				if ((cx - x) * (cx - x) + (cy - y) * (cy - y) < half * half) {
					stencil[i] = 0xff;
				}
				++i;
			}
		}
	} else {
		memset(stencil, 0xff, size * size);
	}
}

Brush brush_create(int size, bool round)
{
	int sbytes = size * size;
	if (sbytes < MIN_STENCIL_BYTES) {
		sbytes = MIN_STENCIL_BYTES;
	}
	uint8_t *stencil = xalloc(sbytes);
	fill_stencil(stencil, size, round);

	return (Brush) {
		.size = size,
		.round = round,
		.stencil = stencil,
		.sbytes = sbytes
	};
}

void brush_set_size(Brush *brush, int new_size)
{
	if (new_size == brush->size) {
		return;
	}

	if (new_size < 1) {
		new_size = 1;
	}
	int sbytes = brush->sbytes;
	int need = new_size * new_size;
	if (need < MIN_STENCIL_BYTES) {
		need = MIN_STENCIL_BYTES;
	}
	if (need * 4 < sbytes) {
		sbytes /= 2;
	}
	while (need > sbytes) {
		sbytes *= 2;
	}
	assert(sbytes >= MIN_STENCIL_BYTES && sbytes >= new_size * new_size);

	if (sbytes != brush->sbytes) {
		brush->sbytes = sbytes;
		brush->stencil = xalloc(sbytes);
	}
	brush->size = new_size;
	fill_stencil(brush->stencil, brush->size, brush->round);
}

void brush_resize(Brush *brush, int delta)
{
	brush_set_size(brush, brush->size + delta);
}

void brush_set_round(Brush *brush, bool round)
{
	if (round != brush->round) {
		brush->round = round;
		fill_stencil(brush->stencil, brush->size, round);
	}
}

void brush_free(Brush *brush)
{
	free(brush->stencil);
}
