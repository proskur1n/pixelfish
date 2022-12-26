// Canvas is only responsible for storing its pixels and managing the undo/redo
// system. It does not implement any drawing operations.
#pragma once

#include <SDL2/SDL_render.h>
#include "palette.h"

typedef struct {
	int w;
	int h;
	SDL_Texture *texture;
	Color *pixels; // [w * h] pixels in RGBA format.
	Color *pixels_backup; // Used inside the history system. Do not edit directly.
} Canvas;

// Pixels must point to a valid heap-allocated [w * h] array. Canvas becomes
// the sole owner of this memory buffer. Do not free pixels yourself.
Canvas canvas_create_from_memory(int w, int h, Color *pixels, SDL_Renderer *ren);

// Creates a new canvas with bg as its background.
Canvas canvas_create_with_background(int w, int h, Color bg, SDL_Renderer *ren);

// Tries to create a canvas with the specified image content. Returns NULL on success and an error
// message on failure. Note that the previous canvas inside 'out_result' is NOT free'd by this
// function.
char const *canvas_open_image(Canvas *out_result, char const *filepath, SDL_Renderer *ren);

// Frees the dynamically allocated memory inside the canvas.
void canvas_free(Canvas c);
