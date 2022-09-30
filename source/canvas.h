// Canvas is only responsible for storing its pixels and managing the undo/redo
// system. It does not implement any drawing operations.
#pragma once

#include <stdint.h>
#include <SDL2/SDL_render.h>

typedef uint32_t Color;
typedef struct Canvas Canvas;

struct Canvas {
	int w;
	int h;
	SDL_Texture *texture;
	Color *pixels; // [w * h] pixels in RGBA format.
	Color *pixels_backup; // Used inside the history system. Do not edit directly.
};

// All canvas_create_ functions will return NULL on failure.
// Pixels must point to a valid heap-allocated [w * h] array. Canvas becomes
// the sole owner of this memory buffer. Do not free pixels yourself.
Canvas *canvas_create_from_memory(int w, int h, Color *pixels, SDL_Renderer *ren);
// Creates a new canvas with bg as its background.
Canvas *canvas_create_with_background(int w, int h, Color bg, SDL_Renderer *ren);
// It is safe to free a NULL canvas.
void canvas_free(Canvas *c);
