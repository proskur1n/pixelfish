#define _GNU_SOURCE // asprintf, vasprintf
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "util.h"
#include "canvas.h"

typedef struct {
	SDL_Color bg;
	SDL_Color fg;
} Theme;

Theme dark_theme = {
	.bg = {30, 30, 30, 255},
	.fg = {255, 255, 255, 255},
};

SDL_Window *win;
SDL_Renderer *ren;
TTF_Font *font;
Palette *palette;
Color left_color;
Color right_color;
Canvas *canvas;
SDL_Texture *checkerboard;
int offset_x;
int offset_y;
float zoom = 15.0f;
bool panning;
bool ui_wants_mouse; // Do not pass click-events to the canvas.
bool ctrl_down;
bool alt_down;
SDL_Point mouse_pos;
bool just_clicked[6];

SDL_Rect render_string(Theme theme, char const *str, int x, int y, int available_height)
{
	SDL_Surface *sur = TTF_RenderUTF8_Shaded(font, str, theme.fg, theme.bg);
	SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, sur);
	SDL_Rect rect = {0};
	if (sur != NULL && tex != NULL) {
		rect = (SDL_Rect) {x, y + (available_height - sur->h) / 2, sur->w, sur->h};
		SDL_RenderCopy(ren, tex, NULL, &rect);
	}
	SDL_FreeSurface(sur);
	if (tex) {
		SDL_DestroyTexture(tex);
	}
	return rect;
}

void fill_rect(Color color, int x, int y, int w, int h)
{
	SDL_Rect rect = {x, y, w, h};
	SDL_SetRenderDrawColor(ren, RED(color), GREEN(color), BLUE(color), 255);
	SDL_RenderFillRect(ren, &rect);
}

void render_canvas(Theme theme)
{
	if (checkerboard == NULL) {
		Color *pixels = xmalloc(canvas->w * canvas->h * sizeof(Color));
		for (int y = 0; y < canvas->h; ++y) {
			for (int x = 0; x < canvas->w; ++x) {
				pixels[y * canvas->w + x] = (x + y) & 1 ? 0xccccccff : 0x555555ff;
			}
		}
		checkerboard = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, canvas->w, canvas->h);
		SDL_UpdateTexture(checkerboard, NULL, pixels, canvas->w * sizeof(Color));
		free(pixels);
	}
	SDL_SetRenderDrawColor(ren, theme.bg.r, theme.bg.g, theme.bg.b, theme.bg.a);
	SDL_RenderClear(ren);
	SDL_Rect rect = {offset_x, offset_y, (int) (canvas->w * zoom), (int) (canvas->h * zoom)};
	SDL_RenderCopy(ren, checkerboard, NULL, &rect);
	// SDL_RenderCopy(ren, canvas->texture, NULL, &rect);
}

// Uses the concept of immediate mode user interface to register clicks.
void render_clickable_color_pin(Color color, int x, int y, int w) {
	SDL_Rect rect = {x, y, w, w};
	if (SDL_PointInRect(&mouse_pos, &rect)) {
		ui_wants_mouse = true;
		if (just_clicked[SDL_BUTTON_LEFT]) {
			left_color = color;
		}
		if (just_clicked[SDL_BUTTON_RIGHT]) {
			right_color = color;
		}
	}

	fill_rect(color, x, y, w, w);
	if (color == left_color || color == right_color) {
		int tiny = 7;
		int padding = 3;
		SDL_Rect tiny_rect = {x + padding, y + padding, tiny, tiny};
		SDL_SetRenderDrawColor(ren, 0, 0, 0, 255); // TODO: Choose contrasting color.
		if (color == left_color) {
			SDL_RenderFillRect(ren, &tiny_rect);
		} else {
			SDL_RenderDrawRect(ren, &tiny_rect);
		}
		tiny_rect.x += tiny + padding;
		if (color == right_color) {
			SDL_RenderFillRect(ren, &tiny_rect);
		} else {
			SDL_RenderDrawRect(ren, &tiny_rect);
		}
	}
}

void render_user_interface(Theme theme)
{
	ui_wants_mouse = false;
	int x = 0;
	int y = 0;
	int color_width = 30;
	for (int i = 0; i < palette->count; ++i) {
		render_clickable_color_pin(palette->colors[i], x, y, color_width);
		y += color_width;
	}

	int winW, winH;
	SDL_GetRendererOutputSize(ren, &winW, &winH);

	Color c[] = {right_color, left_color};
	for (int i = 0; i < 2; ++i) {
		char str[32];
		snprintf(str, LENGTH(str), " #%02X%02X%02X", RED(c[i]), GREEN(c[i]), BLUE(c[i]));
		int x = winW - 80 - color_width;
		int y = winH - color_width * (i + 1);
		fill_rect(c[i], x, y, color_width, color_width);
		render_string(theme, str, x + color_width, y, color_width);
	}

	char status[128];
	snprintf(status, LENGTH(status), " %s", "Hello, world!");
	int tw = 0;
	int th = 0;
	TTF_SizeUTF8(font, status, &tw, &th);
	render_string(theme, status, 0, winH - th, th);
}

void poll_events()
{
	// Reset io state
	for (int i = 0; i < (int) LENGTH(just_clicked); ++i) {
		just_clicked[i] = false;
	}

	SDL_Event e;
	// TODO: Fix strange scroll wheel bug when using SDL_WaitEvent.
	// SDL_WaitEvent(NULL);
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			exit(EXIT_SUCCESS);
		case SDL_MOUSEBUTTONDOWN:
			if ((e.button.button == SDL_BUTTON_MIDDLE) || (ctrl_down && e.button.button == SDL_BUTTON_LEFT)) {
				// TODO: Set hand cursor
				panning = true;
			} else {
				just_clicked[e.button.button] = true;
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if (panning) {
				// TODO: Reset cursor
				panning = false;
			} else {
				just_clicked[e.button.button] = true;
			}
			break;
		case SDL_MOUSEMOTION:
			mouse_pos.x = e.motion.x;
			mouse_pos.y = e.motion.y;
			if (panning) {
				offset_x += e.motion.xrel;
				offset_y += e.motion.yrel;
			}
			break;
		case SDL_MOUSEWHEEL:
			if (ctrl_down) {
				zoom *= MAX(0.5f, 1.0f + e.wheel.y * 0.15f);
			}
			break;
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			if (e.key.keysym.scancode == SDL_SCANCODE_LCTRL) {
				ctrl_down = e.key.type == SDL_KEYDOWN;
			}
			if (e.key.keysym.scancode == SDL_SCANCODE_LALT) {
				alt_down = e.key.type == SDL_KEYDOWN;
			}
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO) || TTF_Init()) {
		fatalSDL("Could not initialize SDL2");
	}

	font = TTF_OpenFont("amiko/Amiko-Regular.ttf", 16);
	if (font == NULL) {
		fatalSDL("Could not open font");
	}
	TTF_SetFontHinting(font, TTF_HINTING_LIGHT);

	win = SDL_CreateWindow("Pixel Art Editor", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_RESIZABLE);
	if (win == NULL) {
		fatalSDL("Could not create window");
	}
	ren = SDL_CreateRenderer(win, -1, 0);
	if (ren == NULL) {
		fatalSDL("Could not create renderer");
	}

	canvas = canvas_create_with_background(40, 30, 0xaa8877ff, ren);
	if (canvas == NULL) {
		fatalSDL("Could not create canvas"); // TODO
	}
	palette = palette_create_default();
	left_color = palette->colors[0];
	right_color = palette->colors[1];

	while (true) {
		poll_events();
		render_canvas(dark_theme);
		render_user_interface(dark_theme);
		SDL_RenderPresent(ren);
	}
}
