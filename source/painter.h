#pragma once

#include <stdint.h>
// TODO: Canvas probably shouldn't directly depend on SDL2.
#include <SDL2/SDL.h>

typedef struct {
    int w;
    int h;
    SDL_Texture *tex;
    uint32_t pixels[]; // [w * h] pixels in RGBA format.
} Canvas;

Canvas *createCanvas(int w, int h, SDL_Renderer *ren);

typedef enum {
	BRUSH_SQUARE,
	BRUSH_ROUND,
	ERASER,
	COLOR_PICKER,
	BUCKET_FILL,
	TOOL_COUNT // Must be the last element.
} Tool;

typedef struct {
	Tool tool;
	int brushSize;
} Painter;

Painter *createPainter();

void painterUseTool(Painter *, Canvas *c, float x, float y, uint32_t mouse, uint32_t mods);

