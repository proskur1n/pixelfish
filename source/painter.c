#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_mouse.h>
#include "painter.h"
#include "util.h"

Canvas *createCanvas(int w, int h, SDL_Renderer *ren)
{
	Canvas *c = xmalloc(sizeof(Canvas) + sizeof(uint32_t) * w * h);
	c->w = w;
	c->h = h;
	memset(c->pixels, 0, w * h * sizeof(uint32_t));

	c->tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, w, h);
	if (c->tex == NULL) {
		fatalSDL("Could not create texture");
	}
	SDL_SetTextureBlendMode(c->tex, SDL_BLENDMODE_BLEND);
	SDL_UpdateTexture(c->tex, NULL, c->pixels, w * sizeof(uint32_t));

	return c;
}

Painter *createPainter()
{
	Painter *p = xmalloc(sizeof(Painter));
	*p = (Painter) {
		.tool = BRUSH_CIRCLE,
		.brushSize = 5,
	};
	return p;
}

static bool inRadius(float x, float y, float cX, float cY, float r)
{
	float d2 = (x - cX) * (x - cX) + (y - cY) * (y - cY);
	return d2 - r * r < 0.01f;
}

static void useSquareBrush(Painter *p, Canvas *c, float _x, float _y, uint32_t mouse)
{
	int x;
	int y;
	if (p->brushSize % 2 == 0) {
		x = (int) roundf(_x);
		y = (int) roundf(_y);
	} else {
		x = (int) _x;
		y = (int) _y;
	}
	int half = p->brushSize / 2;

	x -= half;
	y -= half;

	uint32_t color = 0;
	if (mouse & SDL_BUTTON_LMASK) {
		color = 0xdd8888ff;
	}
	if (mouse & SDL_BUTTON_RMASK) {
		color = 0x8888ddff;
	}

	for (int j = CLAMP(y, 0, c->h); j < CLAMP(y+p->brushSize, 0, c->h); ++j) {
		for (int i = CLAMP(x, 0, c->w); i < CLAMP(x+p->brushSize, 0, c->w); ++i) {
			c->pixels[j * c->w + i] = color;
		}
	}
}

static void useCircleBrush(Painter *p, Canvas *c, float _x, float _y, uint32_t mouse)
{
	int x;
	int y;
	if (p->brushSize % 2 == 0) {
		x = (int) roundf(_x);
		y = (int) roundf(_y);
	} else {
		x = (int) _x;
		y = (int) _y;
	}
	int half = p->brushSize / 2;

	x -= half;
	y -= half;

	uint32_t color = 0;
	if (mouse & SDL_BUTTON_LMASK) {
		color = 0xdd8888ff;
	}
	if (mouse & SDL_BUTTON_RMASK) {
		color = 0x8888ddff;
	}

	for (int j = CLAMP(y, 0, c->h); j < CLAMP(y+p->brushSize, 0, c->h); ++j) {
		for (int i = CLAMP(x, 0, c->w); i < CLAMP(x+p->brushSize, 0, c->w); ++i) {
			if (inRadius(i + 0.5f, j + 0.5f, x + half, y + half, half)) {
				c->pixels[j * c->w + i] = color;
			}
		}
	}
}

void painterUseTool(Painter *p, Canvas *c, float x, float y, uint32_t mouse, uint32_t mods)
{
	assert(x >= 0 && x <= c->w);
	assert(y >= 0 && y <= c->h);
	assert(mouse != 0);

	// TODO
	useCircleBrush(p, c, x, y, mouse);

	// switch (p->tool) {
	// case BRUSH_SQUARE:
	// 	useSquareBrush(p, c, x, y);
	// 	break;
	// }

	// TODO
	SDL_UpdateTexture(c->tex, NULL, c->pixels, c->w * sizeof(uint32_t));
}

