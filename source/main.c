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
int offset_x;
int offset_y;
float zoom = 15.0f;

void render_canvas(Theme theme)
{
	SDL_SetRenderDrawColor(ren, theme.bg.r, theme.bg.g, theme.bg.b, theme.bg.a);
	SDL_RenderClear(ren);
}

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

void render_user_interface(Theme theme)
{
	int x = 0;
	int y = 0;
	int color_width = 30;
	for (int i = 0; i < palette->count; ++i) {
		fill_rect(palette->colors[i], x, y, color_width, color_width);
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

	palette = palette_create_default();
	left_color = palette->colors[0];
	right_color = palette->colors[1];

	while (true) {
		SDL_Event e;
		SDL_WaitEvent(NULL);
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT:
				return EXIT_SUCCESS;
			}
		}

		render_canvas(dark_theme);
		render_user_interface(dark_theme);
		SDL_RenderPresent(ren);
	}
}
