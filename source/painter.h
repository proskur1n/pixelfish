#pragma once

#include <stdint.h>
#include <stdbool.h>
// TODO: Canvas probably shouldn't directly depend on SDL_Texture.
#include <SDL2/SDL.h>

typedef uint32_t Color;

typedef enum {
	UNKNOWN_IMAGE,
	PNG,
	JPG
} ImageExtension;

typedef struct {
	int w;
	int h;
	ImageExtension ext;
	SDL_Texture *tex;
	// [w * h] pixels in RGBA format.
	Color *pixels;
	// Used inside the history system. Do not edit directly.
	Color *undoPixels;
} Canvas;

Canvas *createCanvasWithBackground(int w, int h, Color bg, SDL_Renderer *ren);
// Note that this function can fail and return NULL.
Canvas *createCanvasFromFile(char const *path, SDL_Renderer *ren);
// It is safe to free a NULL canvas.
void freeCanvas(Canvas *);

typedef enum {
	NO_TOOL,
	BRUSH_SQUARE,
	BRUSH_ROUND,
	ERASER,
	COLOR_PICKER,
	BUCKET_FILL,
	TOOL_COUNT // Must be the last element.
} Tool;

typedef struct {
	Tool tool;
	Tool prevTool;
	int brushSize;
	uint32_t leftColor;
	uint32_t rightColor;
	uint8_t activeButton;
} Painter;

Painter *createPainter();
void freePainter(Painter *);

// 'x' and 'y' should be relative to the canvas, not window coordinates.
// 'mods': keyboard modifiers
// 'button': mouse button which triggered this event
void painterMouseDown(Painter *, Canvas *, float x, float y, uint32_t mods, uint8_t button);
void painterMouseMove(Painter *, Canvas *, float x, float y, uint32_t mods);
void painterMouseUp(Painter *, Canvas *, float x, float y, uint32_t mods, uint8_t button);

void painterKeyDown(Painter *, int mods, int scancode, bool repeat);
void painterKeyUp(Painter *, int mods, int scancode, bool repeat);

