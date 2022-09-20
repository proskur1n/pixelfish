#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include "painter.h"
#include "util.h"

SDL_Window *win;
SDL_Renderer *ren;
SDL_Texture *checkerboard;
Canvas *canvas;
Painter *painter;
SDL_Point offset;
float zoom = 15.0f;
bool isMouseDown[6]; // TODO

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

bool isCursorInsideCanvas(int winX, int winY)
{
	int relX = winX - offset.x;
	int relY = winY - offset.y;
	return !(relX < 0 || relY < 0 || relX > canvas->w * zoom || relY > canvas->h * zoom);
}

// If the mouse position (x, y) is inside the canvas, fills 'out' with the
// relative coordinates and returns true. Otherwise, sets 'out' to (0, 0)
// and returns false.
bool windowToCanvas(SDL_FPoint *out, int winX, int winY)
{
	if (isCursorInsideCanvas(winX, winY)) {
		out->x = (winX - offset.x) / zoom;
		out->y = (winY - offset.y) / zoom;
		return true;
	}
	out->x = 0.0f;
	out->y = 0.0f;
	return false;
}

int main(int argc, char **argv)
{
	if (SDL_Init(SDL_INIT_VIDEO)) {
		fatalSDL("Could not initialize SDL2");
	}

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

	SDL_Cursor *crossCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
	SDL_Cursor *defaultCursor = SDL_GetDefaultCursor();

	canvas = createCanvas(40, 30, ren);
	painter = createPainter();
	checkerboard = createCheckerboardTexture(ren, canvas->w, canvas->h);

	while (true) {
		SDL_Event e;
		SDL_WaitEvent(&e);
		switch (e.type) {
		case SDL_QUIT:
			goto quit;
		case SDL_MOUSEBUTTONDOWN:
			// TODO is isMouseDown needed?
			isMouseDown[e.button.button] = true;
			if (e.button.button == SDL_BUTTON_LEFT || e.button.button == SDL_BUTTON_RIGHT) {
				SDL_FPoint point;
				if (windowToCanvas(&point, e.button.x, e.button.y)) {
					uint32_t state = SDL_GetMouseState(NULL, NULL);
					uint32_t mods = SDL_GetModState();
					painterUseTool(painter, canvas, point.x, point.y, state, mods);
				}
			}
			break;
		case SDL_MOUSEBUTTONUP:
			isMouseDown[e.button.button] = false;
			break;
		case SDL_MOUSEMOTION:
			if (e.motion.state & SDL_BUTTON_MMASK) {
				offset.x += e.motion.xrel;
				offset.y += e.motion.yrel;
			}
			if (isCursorInsideCanvas(e.motion.x, e.motion.y)) {
				SDL_SetCursor(crossCursor);
			} else {
				SDL_SetCursor(defaultCursor);
			}
			if (e.motion.state & (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK)) {
				SDL_FPoint point;
				if (windowToCanvas(&point, e.button.x, e.button.y)) {
					uint32_t state = e.motion.state;
					uint32_t mods = SDL_GetModState();
					painterUseTool(painter, canvas, point.x, point.y, state, mods);
				}
			}
			break;
		case SDL_MOUSEWHEEL:
			zoom += e.wheel.y * 0.75f;
			// printf("%d %d\n", e.wheel.x, e.wheel.y);
			break;
		}

		SDL_SetRenderDrawColor(ren, 160, 215, 170, 255);
		SDL_RenderClear(ren);

		SDL_Rect rect = {offset.x, offset.y, (int)(canvas->w * zoom), (int)(canvas->h * zoom)};
		SDL_RenderCopy(ren, checkerboard, NULL, &rect);
		SDL_RenderCopy(ren, canvas->tex, NULL, &rect);

		SDL_RenderPresent(ren);
	}

quit:
	// Close and destroy the win
	SDL_DestroyWindow(win);

	// Clean up
	SDL_Quit();
	return 0;
}
