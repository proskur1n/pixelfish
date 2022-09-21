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
		.tool = BRUSH_ROUND,
		.brushSize = 2,
	};
	return p;
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
	SDL_Rect s;
	assert(SDL_IntersectRect(&bbox, brushArea, &s));
	return s;
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

void painterUseTool(Painter *p, Canvas *c, float x, float y, uint32_t mouse, uint32_t mods)
{
	assert(x >= 0 && x < c->w);
	assert(y >= 0 && y < c->h);
	assert(mouse != 0);

	uint32_t color = 0;
	if (mouse & SDL_BUTTON_LMASK) {
		color = 0xdd8833ff;
	} else {
		color = 0x61dd77ff;
	}

	// TODO
	useRoundBrush(p, c, x, y, color);

	// switch (p->tool) {
	// case BRUSH_SQUARE:
	// 	useSquareBrush(p, c, x, y);
	// 	break;
	// }

	// TODO
	SDL_UpdateTexture(c->tex, NULL, c->pixels, c->w * sizeof(uint32_t));
}

