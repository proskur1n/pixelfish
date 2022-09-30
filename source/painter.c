#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_image.h>
#include "painter.h"
#include "util.h"

// TODO: Move this function to canvas.c

// typedef enum {
// 	UNKNOWN_IMAGE,
// 	PNG,
// 	JPG
// } ImageExtension;

// Canvas *createCanvasFromFile(char const *path, SDL_Renderer *ren)
// {
// 	SDL_Surface *surface = NULL;
// 	ImageExtension ext = UNKNOWN_IMAGE;
// 	char const *extstr = strrchr(path, '.');
// 	if (extstr == NULL) {
// 		return NULL;
// 	}

// 	if (strcasecmp(extstr, ".png") == 0) {
// 		if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
// 			return NULL;
// 		}
// 		ext = PNG;
// 		surface = IMG_Load(path);
// 	} else if (strcasecmp(extstr, ".jpg") == 0 || strcasecmp(extstr, ".jpeg") == 0) {
// 		if (!(IMG_Init(IMG_INIT_JPG) & IMG_INIT_JPG)) {
// 			return NULL;
// 		}
// 		ext = JPG;
// 		surface = IMG_Load(path);
// 	}

// 	if (surface) {
// 		Color *pixels = xmalloc(sizeof(Color) * surface->w * surface->h);
// 		SDL_ConvertPixels(surface->w, surface->h, surface->format->format, surface->pixels,
// 			surface->pitch, SDL_PIXELFORMAT_RGBA8888, pixels, surface->w * sizeof(Color));
// 		Canvas *canvas = createCanvasFromMemory(surface->w, surface->h, pixels, ren);
// 		canvas->ext = ext;
// 		SDL_FreeSurface(surface);
// 		return canvas;
// 	}
// 	return NULL;
// }

Painter *createPainter()
{
	Painter *p = xmalloc(sizeof(Painter));
	*p = (Painter) {
		.tool = BRUSH_SQUARE,
		.prevTool = BRUSH_SQUARE,
		.brushSize = 2,
		.leftColor = 0x4498a3ff,
		.rightColor = 0xa34f44ff
	};
	return p;
}

void freePainter(Painter *p)
{
	free(p);
}

static SDL_Rect getBrushArea(Painter *p, Canvas *c, float fx, float fy)
{
	int x = (int) (p->brushSize & 1 ? fx : roundf(fx));
	int y = (int) (p->brushSize & 1 ? fy : roundf(fy));
	return (SDL_Rect) {x - p->brushSize / 2, y - p->brushSize / 2, p->brushSize, p->brushSize};
}

static SDL_Rect clipBrushAreaToCanvas(Painter *p, Canvas *c, SDL_Rect *brushArea)
{
	SDL_Rect bbox = {0, 0, c->w, c->h};
	SDL_Rect result;
	SDL_IntersectRect(&bbox, brushArea, &result);
	return result;
}

static void useSquareBrush(Painter *p, Canvas *c, float fx, float fy, uint32_t color)
{
	SDL_Rect b = getBrushArea(p, c, fx, fy);
	SDL_Rect s = clipBrushAreaToCanvas(p, c, &b);

	for (int j = s.y; j < s.y + s.h; ++j) {
		for (int i = s.x; i < s.x + s.w; ++i) {
			int index = j * c->w + i;
			assert(index >= 0 && index < c->w * c->h);
			c->pixels[index] = color;
		}
	}
}

static void useRoundBrush(Painter *p, Canvas *c, float fx, float fy, uint32_t color)
{
	SDL_Rect b = getBrushArea(p, c, fx, fy);
	SDL_Rect s = clipBrushAreaToCanvas(p, c, &b);

	for (int j = s.y; j < s.y + s.h; ++j) {
		for (int i = s.x; i < s.x + s.w; ++i) {
			float dx = b.x + b.w * 0.5f - (i + 0.5f);
			float dy = b.y + b.h * 0.5f - (j + 0.5f);
			float eps = 0.01f;

			if (dx * dx + dy * dy < (p->brushSize / 2) * (p->brushSize / 2) + eps) {
				int index = j * c->w + i;
				assert(index >= 0 && index < c->w * c->h);
				c->pixels[index] = color;
			}
		}
	}
}

static uint32_t pickColor(Painter *p, Canvas *c, float fx, float fy)
{
	int x = fx;
	int y = fy;
	if (x < 0 || x >= c->w || y < 0 || y >= c->h) {
		return 0;
	}
	return c->pixels[y * c->w + x];
}

void painterMouseDown(Painter *p, Canvas *c, float x, float y, uint32_t mods, uint8_t button)
{
	if (p->activeButton) {
		return;
	}
	p->activeButton = button;

	if (button == SDL_BUTTON_LEFT) {
		if (p->tool == BRUSH_SQUARE) {
			useSquareBrush(p, c, x, y, p->leftColor);
		} else if (p->tool == BRUSH_ROUND) {
			useRoundBrush(p, c, x, y, p->leftColor);
		} else if (p->tool == ERASER) {
			useSquareBrush(p, c, x, y, 0);
		} else if (p->tool == COLOR_PICKER) {
			uint32_t newColor = pickColor(p, c, x, y);
			if (newColor != 0) {
				p->leftColor = newColor;
			}
		}
	} else if (button == SDL_BUTTON_RIGHT) {
		if (p->tool == BRUSH_SQUARE) {
			useSquareBrush(p, c, x, y, p->rightColor);
		} else if (p->tool == BRUSH_ROUND) {
			useRoundBrush(p, c, x, y, p->rightColor);
		} else if (p->tool == COLOR_PICKER) {
			uint32_t newColor = pickColor(p, c, x, y);
			if (newColor != 0) {
				p->rightColor = newColor;
			}
		}
	}

	// TODO
	SDL_UpdateTexture(c->texture, NULL, c->pixels, c->w * sizeof(uint32_t));
}

void painterMouseMove(Painter *p, Canvas *c, float x, float y, uint32_t mods)
{
	if (!p->activeButton) {
		return;
	}

	if (p->activeButton == SDL_BUTTON_LEFT) {
		if (p->tool == BRUSH_SQUARE) {
			useSquareBrush(p, c, x, y, p->leftColor);
		} else if (p->tool == BRUSH_ROUND) {
			useRoundBrush(p, c, x, y, p->leftColor);
		} else if (p->tool == ERASER) {
			useSquareBrush(p, c, x, y, 0);
		}
	} else if (p->activeButton == SDL_BUTTON_RIGHT) {
		if (p->tool == BRUSH_SQUARE) {
			useSquareBrush(p, c, x, y, p->rightColor);
		} else if (p->tool == BRUSH_ROUND) {
			useRoundBrush(p, c, x, y, p->rightColor);
		}
	}

	// TODO
	SDL_UpdateTexture(c->texture, NULL, c->pixels, c->w * sizeof(uint32_t));
}

void painterMouseUp(Painter *p, Canvas *c, float x, float y, uint32_t mods, uint8_t button)
{
	if (p->activeButton != button) {
		return;
	}
	p->activeButton = 0;

	// TODO: Commit changes to history (only for brushes and eraser).
}

static void setTool(Painter *p, Tool tool)
{
	if (p->activeButton) {
		return;
	}
	p->prevTool = p->tool;
	p->tool = tool;
}

void painterKeyDown(Painter *p, int mods, int scancode, bool repeat)
{
	if (scancode == SDL_SCANCODE_LEFTBRACKET) {
		p->brushSize = MAX(1, p->brushSize - 1);
	} else if (scancode == SDL_SCANCODE_RIGHTBRACKET) {
		p->brushSize += 1;
	}

	if (repeat) {
		return;
	}

	switch (scancode) {
	case SDL_SCANCODE_B:
		if (mods & KMOD_SHIFT) {
			setTool(p, BRUSH_ROUND);
		} else {
			setTool(p, BRUSH_SQUARE);
		}
		break;
	case SDL_SCANCODE_E:
		setTool(p, ERASER);
		break;
	case SDL_SCANCODE_G:
		setTool(p, BUCKET_FILL);
		break;
	case SDL_SCANCODE_LALT:
		setTool(p, COLOR_PICKER);
		break;
	default:
		break;
	}
}

void painterKeyUp(Painter *p, int mods, int scancode, bool repeat)
{
	if (scancode == SDL_SCANCODE_E || scancode == SDL_SCANCODE_G || scancode == SDL_SCANCODE_LALT) {
		p->tool = p->prevTool;
	}
}
