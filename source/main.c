#define _GNU_SOURCE // asprintf
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <math.h>
#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "util.h"
#include "canvas.h"
#include "brush.h"

typedef struct {
	SDL_Color bg;
	SDL_Color fg;
} Theme;

Theme dark_theme = {
	.bg = {30, 30, 30, 255},
	.fg = {255, 255, 255, 255},
};

typedef enum {
	BRUSH_ROUND,
	BRUSH_BLOCK,
	ERASER,
	// COLOR_PICKER,
	// BUCKET_FILL,
	TOOL_COUNT // Must be the last element.
} ToolEnum;

SDL_Window *win;
SDL_Renderer *ren;
TTF_Font *font;
Palette palette;
Color left_color;
Color right_color;
ToolEnum prev_tool = BRUSH_ROUND;
ToolEnum tool = BRUSH_ROUND;
Canvas canvas;
Brush brush;
SDL_Texture *checkerboard;
SDL_Point offset;
float zoom = 15.0f;
bool panning;
bool drawing; // TODO remove
int active_button; // Mouse button used for drawing.
bool ui_wants_mouse; // Do not pass click-events to the canvas.
bool ctrl_down;
bool alt_down;
SDL_Point mouse_pos;
bool just_clicked[6];

// Transforms the relative scaled coordinates to indices inside the canvas's color buffer.
static SDL_Rect get_brush_rect(int size, float fx, float fy)
{
	int x = (int) (size & 1 ? fx : roundf(fx));
	int y = (int) (size & 1 ? fy : roundf(fy));
	SDL_Rect rect = {x - size / 2, y - size / 2, size, size};
	return rect;
}

static SDL_Rect render_string(Theme theme, char const *str, int x, int y, int available_height)
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

static void fill_rect(Color color, int x, int y, int w, int h)
{
	SDL_Rect rect = {x, y, w, h};
	SDL_SetRenderDrawColor(ren, RED(color), GREEN(color), BLUE(color), 255);
	SDL_RenderFillRect(ren, &rect);
}

static void render_canvas(Theme theme)
{
	if (checkerboard == NULL) {
		Color *pixels = xalloc(canvas.w * canvas.h * sizeof(Color));
		for (int y = 0; y < canvas.h; ++y) {
			for (int x = 0; x < canvas.w; ++x) {
				pixels[y * canvas.w + x] = (x + y) & 1 ? 0xccccccff : 0x555555ff;
			}
		}
		checkerboard = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, canvas.w, canvas.h);
		SDL_UpdateTexture(checkerboard, NULL, pixels, canvas.w * sizeof(Color));
		free(pixels);
	}
	SDL_SetRenderDrawColor(ren, theme.bg.r, theme.bg.g, theme.bg.b, theme.bg.a);
	SDL_RenderClear(ren);
	SDL_Rect rect = {offset.x, offset.y, (int) (canvas.w * zoom), (int) (canvas.h * zoom)};
	SDL_RenderCopy(ren, checkerboard, NULL, &rect);
	SDL_RenderCopy(ren, canvas.texture, NULL, &rect);
}

static void render_clickable_color_pin(Color color, int x, int y, int w)
{
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

static inline bool is_brush(int x, int y)
{
	if (x < 0 || y < 0 || x >= brush.size || y >= brush.size) {
		return false;
	}
	return brush.stencil[y * brush.size + x];
}

static void render_brush_outline(void)
{
	// TODO: Decide on the blend factor / operation.
	SDL_BlendMode oldBlend = {0};
	SDL_GetRenderDrawBlendMode(ren, &oldBlend);
	SDL_BlendMode mode = SDL_ComposeCustomBlendMode(
		SDL_BLENDFACTOR_ONE_MINUS_DST_COLOR,
		// SDL_BLENDFACTOR_ZERO,
		SDL_BLENDFACTOR_ZERO,
		SDL_BLENDOPERATION_ADD,
		SDL_BLENDFACTOR_ONE,
		SDL_BLENDFACTOR_ONE,
		SDL_BLENDOPERATION_ADD);
	if (SDL_SetRenderDrawBlendMode(ren, mode) < 0) {
		fatalSDL("SDL_SetRenderDrawBlendMode");
	}

	float fx = (mouse_pos.x - offset.x) / zoom;
	float fy = (mouse_pos.y - offset.y) / zoom;
	SDL_Rect brect = get_brush_rect(brush.size, fx, fy);
	int thickness = 2;

	int qy = brect.y * zoom + offset.y;
	for (int y = 0; y <= brush.size; ++y) {
		int qx = brect.x * zoom + offset.x;
		for (int x = 0; x <= brush.size; ++x) {
			bool cur = is_brush(x, y);
			if (is_brush(x - 1, y) != cur) {
				// TODO: FIX: too many state changes because of fill_rect
				fill_rect(0xffffffff, qx, qy, cur ? -thickness : thickness, zoom);
			}
			if (is_brush(x, y - 1) != cur) {
				fill_rect(0xffffffff, qx, qy, zoom, cur ? -thickness : thickness);
			}
			qx += zoom;
		}
		qy += zoom;
	}

	SDL_SetRenderDrawBlendMode(ren, oldBlend);
}

static void render_user_interface(Theme theme)
{
	if (!ui_wants_mouse || drawing) {
		render_brush_outline();
	}

	ui_wants_mouse = false;
	int x = 0;
	int y = 0;
	int color_width = 30;
	for (int i = 0; i < palette.count; ++i) {
		render_clickable_color_pin(palette.colors[i], x, y, color_width);
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
	snprintf(status, LENGTH(status), " Brush size: %d, History: [%d/%d]", brush.size, canvas.undo_left, canvas.undo_left + canvas.redo_left);
	int tw = 0;
	int th = 0;
	TTF_SizeUTF8(font, status, &tw, &th);
	render_string(theme, status, 0, winH - th, th);
}

static void use_brush(bool round, int size, Color color, float fx, float fy)
{
	SDL_Rect rect = get_brush_rect(size, fx, fy);
	SDL_Rect bbox = {0, 0, canvas.w, canvas.h};
	SDL_Rect clip;
	SDL_IntersectRect(&bbox, &rect, &clip);

	for (int y = clip.y; y < clip.y + clip.h; ++y) {
		for (int x = clip.x; x < clip.x + clip.w; ++x) {
			int si = (y - rect.y) * brush.size + (x - rect.x);
			assert(si < brush.size * brush.size);
			if (brush.stencil[si]) {
				int index = y * canvas.w + x;
				assert(index >= 0 && index < canvas.w * canvas.h);
				canvas.pixels[index] = color;
			}
		}
	}

	canvas_mark_dirty(&canvas, clip);
}

static void tool_on_click(int button)
{
	float fx = (mouse_pos.x - offset.x) / zoom;
	float fy = (mouse_pos.y - offset.y) / zoom;
	Color color = button == SDL_BUTTON_LEFT ? left_color : right_color;

	switch (tool) {
	case BRUSH_ROUND:
		use_brush(true, brush.size, color, fx, fy);
		break;
	case BRUSH_BLOCK:
		use_brush(false, brush.size, color, fx, fy);
		break;
	case ERASER:
		use_brush(true, brush.size, 0, fx, fy);
	case TOOL_COUNT:
		break;
	}
}

static void tool_on_move(void)
{
	if (tool == BRUSH_ROUND || tool == BRUSH_BLOCK || tool == ERASER) {
		tool_on_click(active_button);
	}
}

static void poll_events(void)
{
	// Reset io state
	for (size_t i = 0; i < LENGTH(just_clicked); ++i) {
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
				if (!ui_wants_mouse) {
					drawing = true;
					active_button = e.button.button;
					tool_on_click(active_button);
				}
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if (panning) {
				// TODO: Reset cursor
				panning = false;
			} else if (e.button.button == active_button) {
				if (drawing) {
					canvas_commit(&canvas);
				}
				drawing = false;
				active_button = 0;
			}
			break;
		case SDL_MOUSEMOTION:
			mouse_pos.x = e.motion.x;
			mouse_pos.y = e.motion.y;
			if (panning) {
				offset.x += e.motion.xrel;
				offset.y += e.motion.yrel;
			} else if (drawing) {
				tool_on_move();
			}
			break;
		case SDL_MOUSEWHEEL:
			if (ctrl_down) {
				zoom *= MAX(0.5f, 1.0f + e.wheel.y * 0.15f);
			} else {
				brush_resize(&brush, e.wheel.y);
			}
			break;
		case SDL_KEYDOWN:
			// TODO: Think of a better input handling system :/
			if (e.key.keysym.scancode == SDL_SCANCODE_E) {
				if (tool != ERASER) {
					prev_tool = tool;
					tool = ERASER;
				}
			} else if (e.key.keysym.scancode == SDL_SCANCODE_LCTRL) {
				ctrl_down = true;
			} else if (e.key.keysym.scancode == SDL_SCANCODE_LALT) {
				alt_down = true;
			} else if (e.key.keysym.scancode == SDL_SCANCODE_Z) {
				if (e.key.keysym.mod & KMOD_LCTRL) {
					canvas_undo(&canvas);
				}
			} else if (e.key.keysym.scancode == SDL_SCANCODE_Y) {
				if (e.key.keysym.mod & KMOD_LCTRL) {
					canvas_redo(&canvas);
				}
			}
			break;
		case SDL_KEYUP:
			if (e.key.keysym.scancode == SDL_SCANCODE_LCTRL) {
				ctrl_down = e.key.type == SDL_KEYDOWN;
			} else if (e.key.keysym.scancode == SDL_SCANCODE_LALT) {
				alt_down = e.key.type == SDL_KEYDOWN;
			} else if (e.key.keysym.scancode == SDL_SCANCODE_E) {
				tool = prev_tool;
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

	canvas = canvas_create_with_background(40, 30, 0, ren);
	// char const *msg = canvas_open_image(&canvas, "Elfst33.jpg", ren);
	// if (msg != NULL) {
		// fatal("Could not open image: %s", msg);
	// }

	brush = brush_create(5, true);
	palette = palette_get_default();
	left_color = palette.colors[0];
	right_color = palette.colors[1];

	while (true) {
		poll_events();
		render_canvas(dark_theme);
		render_user_interface(dark_theme);
		SDL_RenderPresent(ren);
	}
}
