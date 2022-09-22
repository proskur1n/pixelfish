#define _GNU_SOURCE
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "painter.h"
#include "util.h"

TTF_Font *font;
SDL_Window *win;
SDL_Renderer *ren;
SDL_Texture *checkerboard;
Canvas *canvas;
Painter *painter;
SDL_Point offset;
float zoom = 15.0f;
bool panning = false;

char const *toolName[TOOL_COUNT] = {
	"Unknown",
	"Square brush",
	"Round brush",
	"Eraser",
	"Color picker",
	"Bucket fill"
};

SDL_Texture *createCheckerboardTexture(SDL_Renderer *ren, int w, int h)
{
	SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, w, h);
	if (tex == NULL) {
		fatalSDL("Could not create texture");
	}

	uint32_t *pixels = xmalloc(w * h * sizeof(uint32_t));
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			pixels[y * w + x] = (x + y) & 1 ? 0xccccccff : 0x555555ff;
		}
	}

	if (SDL_UpdateTexture(tex, NULL, pixels, w * sizeof(uint32_t))) {
		fatalSDL("SDL_UpdateTexture");
	}

	free(pixels);
	return tex;
}

// Converts winX and winY from window coordinates to the relative
// canvas coordinates.
SDL_FPoint windowToCanvas(int winX, int winY)
{
	return (SDL_FPoint) { (winX - offset.x) / zoom, (winY - offset.y) / zoom };
}

void showString(char const *format, ...)
{
	char *str = NULL;
	va_list va;
	va_start(va, format);
	if (vasprintf(&str, format, va) < 0) {
		return;
	}
	va_end(va);

	SDL_Color fg = {255, 255, 255, 255};
	SDL_Color bg = {30, 30, 30, 255};
	SDL_Surface *s = TTF_RenderUTF8_Shaded(font, str, fg, bg);
	free(str);
	if (s == NULL) {
		return;
	}
	SDL_Texture *t = SDL_CreateTextureFromSurface(ren, s);
	if (t == NULL) {
		SDL_FreeSurface(s);
		return;
	}

	int winW, winH;
	SDL_GetRendererOutputSize(ren, &winW, &winH);
	int paddingLeft = 4;

	SDL_Rect dest = {paddingLeft, winH - s->h, s->w, s->h};
	SDL_RenderCopy(ren, t, NULL, &dest);
	SDL_FreeSurface(s);
	SDL_DestroyTexture(t);
}

int main(int argc, char **argv)
{
	if (SDL_Init(SDL_INIT_VIDEO)) {
		fatalSDL("Could not initialize SDL2");
	}

	if (TTF_Init()) {
		fatalSDL("Could not initialize SDL2_ttf");
	}

	font = TTF_OpenFont("amiko/Amiko-Regular.ttf", 16);
	if (font == NULL) {
		fatalSDL("Could not open font");
	}
	TTF_SetFontHinting(font, TTF_HINTING_LIGHT);

	win = SDL_CreateWindow(
		"An SDL2 win",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		640,
		480,
		SDL_WINDOW_RESIZABLE
	);
	if (win == NULL) {
		fatal("Could not create win: %s", SDL_GetError());
	}

	ren = SDL_CreateRenderer(win, -1, 0);
	if (!ren) {
		fatal("Could not create renderer: %s", SDL_GetError());
	}

	// SDL_RendererInfo info;
	// if (SDL_GetRendererInfo(ren, &info)) {
	// 	fatalSDL("SDL_GetRendererInfo");
	// }
	// printf("software: %d\n", info.flags & SDL_RENDERER_SOFTWARE);
	// printf("accelerated: %d\n", info.flags & SDL_RENDERER_ACCELERATED);
	// printf("vsync: %d\n", info.flags & SDL_RENDERER_PRESENTVSYNC);
	// printf("targettexture: %d\n", info.flags & SDL_RENDERER_TARGETTEXTURE);

	SDL_Cursor *crossCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
	SDL_Cursor *handCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	SDL_Cursor *defaultCursor = SDL_GetDefaultCursor();

	canvas = createCanvas(60, 50, ren);
	painter = createPainter();
	checkerboard = createCheckerboardTexture(ren, canvas->w, canvas->h);

	while (true) {
		SDL_WaitEvent(NULL); // This will call SDL_PumpEvents.
		SDL_Event events[16];
		int numEvents = SDL_PeepEvents(events, LENGTH(events), SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);

		int x, y;
		uint32_t mouseState = SDL_GetMouseState(&x, &y);
		uint32_t mods = SDL_GetModState();

		// while (SDL_PollEvent(&e)) {
		for (int i = 0; i < numEvents; ++i) {
			SDL_Event e = events[i];
			switch (e.type) {
			case SDL_QUIT:
				goto quit;
			case SDL_KEYDOWN:
				painterKeyDown(painter, mods, e.key.keysym.scancode, e.key.repeat);
				break;
			case SDL_KEYUP:
				painterKeyUp(painter, mods, e.key.keysym.scancode, e.key.repeat);
				break;
			case SDL_MOUSEWHEEL:
				if (mods & KMOD_CTRL) {
					zoom *= MAX(0.5f, 1.0f + e.wheel.y * 0.15f);
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				if ((mouseState & SDL_BUTTON_MMASK) || ((mods & KMOD_CTRL) && e.button.button == SDL_BUTTON_LEFT)) {
					if (!panning) {
						SDL_SetCursor(handCursor);
						panning = true;
					}
				} else {
					SDL_FPoint xy = windowToCanvas(x, y);
					painterMouseDown(painter, canvas, xy.x, xy.y, mods, e.button.button);
				}
				break;
			case SDL_MOUSEBUTTONUP:
				if (panning) {
					SDL_SetCursor(crossCursor);
					panning = false;
				} else {
					SDL_FPoint xy = windowToCanvas(x, y);
					painterMouseUp(painter, canvas, xy.x, xy.y, mods, e.button.button);
				}
				break;
			case SDL_MOUSEMOTION:
				if (panning) {
					offset.x += e.motion.xrel;
					offset.y += e.motion.yrel;
				} else {
					SDL_FPoint xy = windowToCanvas(x, y);
					painterMouseMove(painter, canvas, xy.x, xy.y, mods);
				}
				break;
			}
		}

		SDL_SetRenderDrawColor(ren, 30, 30, 30, 255);
		SDL_RenderClear(ren);

		SDL_Rect rect = {offset.x, offset.y, (int)(canvas->w * zoom), (int)(canvas->h * zoom)};
		SDL_RenderCopy(ren, checkerboard, NULL, &rect);
		SDL_RenderCopy(ren, canvas->tex, NULL, &rect);

		showString("%s (%d)", toolName[painter->tool], painter->brushSize);

		SDL_RenderPresent(ren);
	}

quit:
	// TODO: Clean up everything.
	SDL_DestroyWindow(win);
	SDL_Quit();
}

