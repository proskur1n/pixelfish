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
	uint32_t leftColor;
	uint32_t rightColor;
	uint8_t activeButton;
} Painter;

Painter *createPainter();

// 'x' and 'y' should be relative to the canvas, not window coordinates.
// 'mods': keyboard modifiers
// 'button': mouse button which triggered this event
void painterMouseDown(Painter *, Canvas *, float x, float y, uint32_t mods, uint8_t button);
void painterMouseMove(Painter *, Canvas *, float x, float y, uint32_t mods);
void painterMouseUp(Painter *, Canvas *, float x, float y, uint32_t mods, uint8_t button);

