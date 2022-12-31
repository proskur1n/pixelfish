#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "util.h"
#include "canvas.h"
#include "brush.h"
#include "dialog.h"
#include "embed.h"

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
	BRUSH_SQUARE,
	ERASER,
	COLOR_PICKER,
	BUCKET_FILL,
	TOOL_COUNT // Must be the last element.
} ToolEnum;

bool running = true;
SDL_Window *win;
SDL_Renderer *ren;
TTF_Font *font;
Color left_color;
Color right_color;
ToolEnum prev_tool = BRUSH_ROUND;
ToolEnum tool = BRUSH_ROUND;
Canvas canvas;
Brush brush;
SDL_Texture *checkerboard;
SDL_Point offset;
float zoom = 15.0f; // One image pixel takes up "zoom" pixels on the screen.
bool panning;
bool drawing;
int active_button; // Mouse button used for drawing.
bool ui_wants_mouse; // Do not pass click-events to the canvas.
SDL_Point mouse_pos;
bool just_clicked[6];
char error_text[128]; // Error message displayed in the bottom-left corner.
Uint64 error_timeout; // Timestamp after which the error_text should disappear.

// Kudos: NA16 by Nauris (https://lospec.com/palette-list/na16)
static Color const default_palette[] = {
	0x8c8faeff, 0x584563ff, 0x3e2137ff, 0x9a6348ff,
	0xd79b7dff, 0xf5edbaff, 0xc0c741ff, 0x647d34ff,
	0xe4943aff, 0x9d303bff, 0xd26471ff, 0x70377fff,
	0x7ec4c1ff, 0x34859dff, 0x17434bff, 0x1f0e1cff,
};
// TODO: Add a way to load different palettes.

// Transforms the relative scaled coordinates to indices inside the canvas's color buffer.
static SDL_Rect get_brush_rect(int size, float fx, float fy)
{
	int x = (int) (size & 1 ? floorf(fx) : roundf(fx));
	int y = (int) (size & 1 ? floorf(fy) : roundf(fy));
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

static void render_brush_outline(void)
{
	static SDL_BlendMode inverted_blend = 0;
	if (inverted_blend == 0) {
		inverted_blend = SDL_ComposeCustomBlendMode(
			SDL_BLENDFACTOR_ONE_MINUS_DST_COLOR, SDL_BLENDFACTOR_ZERO, SDL_BLENDOPERATION_ADD,
			SDL_BLENDFACTOR_ONE_MINUS_DST_COLOR, SDL_BLENDFACTOR_ZERO, SDL_BLENDOPERATION_ADD
		);
	}

	SDL_BlendMode old_blend = 0;
	SDL_GetRenderDrawBlendMode(ren, &old_blend);

	if (SDL_SetRenderDrawBlendMode(ren, inverted_blend) < 0) {
		// Fallback to a simpler blend mode.
		SDL_SetRenderDrawColor(ren, 0, 0, 0, 0xff);
	} else {
		SDL_SetRenderDrawColor(ren, 0xff, 0xff, 0xff, 0xff);
	}

	float fx = (mouse_pos.x - offset.x) / zoom;
	float fy = (mouse_pos.y - offset.y) / zoom;
	SDL_Rect brect = get_brush_rect(brush.size, fx, fy);
	int const thickness = 2;

	// Horizontal lines
	for (int y = 0; y <= brush.size; ++y) {
		int line_x = INT_MIN;
		int line_y = offset.y + (brect.y + y) * zoom;
		bool left = false;
		for (int x = 0; x <= brush.size; ++x) {
			bool cur = x < brush.size && y < brush.size && brush.stencil[y * brush.size + x];
			bool top = y > 0 && x < brush.size && brush.stencil[(y - 1) * brush.size + x];
			if (line_x != INT_MIN && (cur == top || cur != left)) {
				// Finish previous line
				int width = (offset.x + (brect.x + x) * zoom) - line_x;
				if (!cur) {
					width += thickness;
				}
				SDL_Rect rect = {line_x, line_y, width, left ? -thickness : thickness};
				SDL_RenderFillRect(ren, &rect);
				line_x = INT_MIN;
			}
			if (cur != top && line_x == INT_MIN) {
				// Start a new line
				line_x = offset.x + (brect.x + x) * zoom;
				if (!left) {
					line_x -= thickness;
				}
			}
			left = cur;
		}
	}

	// Vertical lines
	for (int x = 0; x <= brush.size; ++x) {
		int line_x = offset.x + (brect.x + x) * zoom;
		int line_y = INT_MIN;
		bool top = false;
		for (int y = 0; y <= brush.size; ++y) {
			bool cur = x < brush.size && y < brush.size && brush.stencil[y * brush.size + x];
			bool left = x > 0 && y < brush.size && brush.stencil[y * brush.size + x - 1];
			if (line_y != INT_MIN && (cur == left || cur != top)) {
				// Finish previous line
				int height = (offset.y + (brect.y + y) * zoom) - line_y;
				if (cur) {
					height -= thickness;
				}
				SDL_Rect rect = {line_x, line_y, top ? -thickness : thickness, height};
				SDL_RenderFillRect(ren, &rect);
				line_y = INT_MIN;
			}
			if (cur != left && line_y == INT_MIN) {
				// Start a new line
				line_y = offset.y + (brect.y + y) * zoom;
				if (top) {
					line_y += thickness;
				}
			}
			top = cur;
		}
	}

	SDL_SetRenderDrawBlendMode(ren, old_blend);
}

static void render_user_interface(Theme theme)
{
	if (!panning && (!ui_wants_mouse || drawing) && tool <= ERASER) {
		render_brush_outline();
	}

	int winW, winH;
	SDL_GetRendererOutputSize(ren, &winW, &winH);
	ui_wants_mouse = false;
	int x = 0;
	int y = 0;
	int color_width = 30;
	for (size_t i = 0; i < LENGTH(default_palette); ++i) {
		if (y + color_width > winH * 8 / 10) {
			x += color_width;
			y = 0;
		}
		render_clickable_color_pin(default_palette[i], x, y, color_width);
		y += color_width;
	}

	Color c[] = {right_color, left_color};
	for (int i = 0; i < 2; ++i) {
		char str[32];
		snprintf(str, LENGTH(str), " #%02X%02X%02X", RED(c[i]), GREEN(c[i]), BLUE(c[i]));
		int x = winW - 80 - color_width;
		int y = winH - color_width * (i + 1);
		fill_rect(c[i], x, y, color_width, color_width);
		render_string(theme, str, x + color_width, y, color_width);
	}

	char const * const tool_name[TOOL_COUNT] = {
		[BRUSH_ROUND] = "Round brush",
		[BRUSH_SQUARE] = "Square brush",
		[ERASER] = "Eraser",
		[COLOR_PICKER] = "Color picker",
		[BUCKET_FILL] = "Bucket",
	};

	char status[128];
	if (SDL_GetTicks64() < error_timeout) {
		strncpy(status, error_text, LENGTH(status));
		status[LENGTH(status) - 1] = 0;
	} else {
		int len = 0;
		if (tool <= ERASER) {
			len += sprintf(status, "%s (%d)", tool_name[tool], brush.size);
		} else {
			len += sprintf(status, "%s", tool_name[tool]);
		}
		sprintf(status + len, " | History: %d/%d%s", canvas.undo_left,
			canvas.undo_left + canvas.redo_left, canvas.unsaved ? " [ + ]" : "");
	}

	int padding = 4;
	int tw = 0;
	int th = 0;
	TTF_SizeUTF8(font, status, &tw, &th);
	fill_rect(COLOR_FROM(theme.bg), 0, winH - th, tw + padding * 2, th);
	render_string(theme, status, padding, winH - th, th);
}

static void show_error(char const *format, ...) __attribute__ ((format (printf, 1, 2)));
static void show_error(char const *format, ...)
{
	va_list va;
	va_start(va, format);
	vsnprintf(error_text, LENGTH(error_text), format, va);
	va_end(va);
	error_timeout = SDL_GetTicks64() + 2500;
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

static void pick_color(Color *out, float fx, float fy)
{
	int x = (int) fx;
	int y = (int) fy;
	if (x >= 0 && y >= 0 && x < canvas.w && y < canvas.h) {
		*out = canvas.pixels[y * canvas.w + x];
	}
}

// Internal function for bucket_fill.
static void bucket_fill__scan(int from, int to, int y, Color replace_this, SDL_Point *stack, int stack_cap, int *s)
{
	bool span_added = false;
	for (int x = from; x <= to; ++x) {
		if (canvas.pixels[y * canvas.w + x] != replace_this) {
			span_added = false;
		} else if (!span_added) {
			if (*s + 1 > stack_cap) {
				return;
			}
			stack[(*s)++] = (SDL_Point) {x, y};
			span_added = true;
		}
	}
}

// A moderately efficient Span-Filling algorithm from Wikipedia
// https://en.wikipedia.org/wiki/Flood_fill#Span_Filling
static void bucket_fill(float fx, float fy, Color color)
{
	int x = (int) fx;
	int y = (int) fy;
	if (x < 0 || y < 0 || x >= canvas.w || y >= canvas.h) {
		return;
	}

	enum { STACK_CAP = 128 };
	SDL_Point stack[STACK_CAP];
	int s = 0;
	Color replace_this = canvas.pixels[y * canvas.w + x];
	int minX = x, maxX = x;
	int minY = y, maxY = y;
	stack[s++] = (SDL_Point) {x, y};

	if (replace_this == color) {
		// Fast out, replacing a color by itself has no effect.
		return;
	}

	while (s > 0) {
		SDL_Point p = stack[--s];

		int from = p.x;
		while (from > 0 && canvas.pixels[p.y * canvas.w + from - 1] == replace_this) {
			canvas.pixels[p.y * canvas.w + from - 1] = color;
			--from;
		}

		int to = p.x;
		while (to < canvas.w && canvas.pixels[p.y * canvas.w + to] == replace_this) {
			canvas.pixels[p.y * canvas.w + to] = color;
			++to;
		}
		--to;

		if (from < minX) {
			minX = from;
		}
		if (to > maxX) {
			maxX = to;
		}
		if (p.y < minY) {
			minY = p.y;
		} else if (p.y > maxY) {
			maxY = p.y;
		}

		if (p.y > 0) {
			bucket_fill__scan(from, to, p.y - 1, replace_this, stack, STACK_CAP, &s);
		}
		if (p.y + 1 < canvas.h) {
			bucket_fill__scan(from, to, p.y + 1, replace_this, stack, STACK_CAP, &s);
		}
	}

	SDL_Rect changed = {minX, minY, maxX - minX + 1, maxY - minY + 1};
	canvas_mark_dirty(&canvas, changed);
}

static void tool_on_click(int button)
{
	if (button != SDL_BUTTON_LEFT && button != SDL_BUTTON_RIGHT) {
		return;
	}
	float fx = (mouse_pos.x - offset.x) / zoom;
	float fy = (mouse_pos.y - offset.y) / zoom;
	Color color = button == SDL_BUTTON_LEFT ? left_color : right_color;

	switch (tool) {
	case BRUSH_ROUND:
		use_brush(true, brush.size, color, fx, fy);
		break;
	case BRUSH_SQUARE:
		use_brush(false, brush.size, color, fx, fy);
		break;
	case ERASER:
		use_brush(true, brush.size, 0, fx, fy);
		break;
	case COLOR_PICKER:
		if (button == SDL_BUTTON_LEFT) {
			pick_color(&left_color, fx, fy);
		} else {
			pick_color(&right_color, fx, fy);
		}
		break;
	case BUCKET_FILL:
		if (button == SDL_BUTTON_LEFT) {
			bucket_fill(fx, fy, left_color);
		} else {
			bucket_fill(fx, fy, right_color);
		}
		break;
	case TOOL_COUNT:
		unreachable();
	}
}

static void tool_on_move(void)
{
	if (tool == BRUSH_ROUND || tool == BRUSH_SQUARE || tool == ERASER) {
		tool_on_click(active_button);
	}
}

// Set the current cursor to the specified cursor type. This function has a cache of cursors for
// better performance. You can pass a negative value for cursor to free this internal cache.
static void set_cursor(SDL_SystemCursor cursor)
{
	static SDL_Cursor *cache[SDL_NUM_SYSTEM_CURSORS];
	static SDL_SystemCursor current = SDL_SYSTEM_CURSOR_ARROW;

	if (cursor < 0) {
		// Free all allocated cursors.
		for (int i = 0; i < SDL_NUM_SYSTEM_CURSORS; ++i) {
			if (cache[i] != NULL) {
				SDL_FreeCursor(cache[i]);
			}
		}
		return;
	}

	assert(cursor >= 0 && cursor < SDL_NUM_SYSTEM_CURSORS);
	if (cache[cursor] == NULL) {
		cache[cursor] = SDL_CreateSystemCursor(cursor);
	}
	if (current != cursor) {
		SDL_SetCursor(cache[cursor]);
		current = cursor;
	}
}

// Makes the canvas visible again if it goes too far off the screen.
static void constrain_canvas(void)
{
	int w, h;
	SDL_GetWindowSize(win, &w, &h);
	int const min_visible_pixels = 40;

	if (offset.x + canvas.w * zoom < min_visible_pixels) {
		offset.x = min_visible_pixels - canvas.w * zoom;
	}
	if (w - offset.x < min_visible_pixels) {
		offset.x = w - min_visible_pixels;
	}

	if (offset.y + canvas.h * zoom < min_visible_pixels) {
		offset.y = min_visible_pixels - canvas.h * zoom;
	}
	if (h - offset.y < min_visible_pixels) {
		offset.y = h - min_visible_pixels;
	}
}

// Amount can be both positive and negative.
static void change_zoom(int amount)
{
	int x, y;
	SDL_GetMouseState(&x, &y);

	float mult = 1.0f + amount * 0.15f;
	if (mult < 0.5f) {
		mult = 0.5f;
	}

	float old_zoom = zoom;
	zoom *= mult;
	if (zoom > 45.0f) {
		zoom = 45.0f;
	} else if (zoom < 2.0f) {
		zoom = 2.0f;
	}

	offset.x = x - (x - offset.x) * zoom / old_zoom;
	offset.y = y - (y - offset.y) * zoom / old_zoom;
	constrain_canvas();
}

typedef enum { SAVE, SAVE_AS } SaveMethod;

static bool save_file(SaveMethod method)
{
	CanvasFileStatus status;
	if (method == SAVE) {
		status = canvas_save_to_file(&canvas, NULL);
	} else {
		status = canvas_save_as_to_file(&canvas);
	}
	switch (status) {
	case CF_OK:
		return true;
	case CF_CANCELLED_BY_USER:
		break;
	case CF_UNKNOWN_IMAGE_FORMAT:
		show_error("Could not save image: Unsupported image format");
		break;
	case CF_OTHER_ERROR:
		show_error("Could not save image: I/O error");
		break;
	}
	return false;
}

static bool can_close_canvas(void)
{
	if (!canvas.unsaved) {
		return true;
	}
	switch (dialog_unsaved_changes_confirmation()) {
	case DIALOG_RESPONSE_CANCEL:
		return false;
	case DIALOG_RESPONSE_SAVE:
		return save_file(SAVE);
	case DIALOG_RESPONSE_DISCARD:
		return true;
	}
	unreachable();
}

static void try_quit_application(void)
{
#ifdef DEVELOPER
	// Speed up the edit-compile-debug cycle in debug mode.
	running = false;
#else
	if (can_close_canvas()){
		running = false;
	}
#endif
}

static void set_canvas(Canvas new_canvas)
{
	canvas_free(canvas);
	canvas = new_canvas;
	if (checkerboard != NULL) {
		SDL_DestroyTexture(checkerboard);
		checkerboard = NULL;
	}
	// TODO: Center / Resize canvas
}

enum {
	ALLOW_REPEAT = 1u << 0,
};

typedef union {
	int i;
} Arg;

typedef void (*KeyActionFunc)(Arg arg, SDL_Keycode key, uint16_t mod);

typedef struct {
	SDL_Keycode key;
	uint16_t mod;
	uint16_t flag;
	KeyActionFunc func;
	Arg arg;
} KeyAction;

static void ka_zoom(Arg arg, SDL_Keycode key, uint16_t mod)
{
	change_zoom(arg.i);
}

static void ka_change_tool(Arg arg, SDL_Keycode key, uint16_t mod)
{
	if (arg.i < 0) {
		tool = prev_tool;
	} else if ((int) tool != arg.i) {
		prev_tool = tool;
		tool = arg.i;
	}
	if (tool == BRUSH_ROUND || (tool == ERASER && prev_tool == BRUSH_ROUND)) {
		brush_set_round(&brush, true);
	} else if (tool == BRUSH_SQUARE || tool == ERASER) {
		brush_set_round(&brush, false);
	}
}

static void ka_brush_size(Arg arg, SDL_Keycode key, uint16_t mod)
{
	brush_resize(&brush, arg.i);
}

static void ka_undo_redo(Arg arg, SDL_Keycode key, uint16_t mod)
{
	if (arg.i < 0) {
		canvas_undo(&canvas);
	} else {
		canvas_redo(&canvas);
	}
}

static void ka_save_file(Arg arg, SDL_Keycode key, uint16_t mod)
{
	save_file(arg.i);
}

static void ka_open_file(Arg arg, SDL_Keycode key, uint16_t mod)
{
	if (!can_close_canvas()) {
		return;
	}

	char *filepath = dialog_open_file(NULL);
	if (filepath == NULL) {
		return;
	}

	Canvas new_canvas;
	char const *err = canvas_open_image(&new_canvas, filepath, ren);
	free(filepath);
	if (err == NULL) {
		set_canvas(new_canvas);
	} else {
		show_error("Could not open image: %s", err);
	}
}

static void ka_new_file(Arg arg, SDL_Keycode key, uint16_t mod)
{
	if (!can_close_canvas()) {
		return;
	}
	int width = 0;
	int height = 0;
	if (dialog_width_and_height(&width, &height)) {
		if (width <= 0 || height <= 0) {
			show_error("Invalid image size (%d, %d)", width, height);
		} else {
			set_canvas(canvas_create_with_background(width, height, 0, ren));
		}
	}
}

static void ka_quit(Arg arg, SDL_Keycode key, uint16_t mod)
{
	(void) arg;
	try_quit_application();
}

static KeyAction const key_down_actions[] = {
	{ SDLK_MINUS,   0,           ALLOW_REPEAT, ka_zoom,        {.i = -1} },
	{ SDLK_EQUALS,  0,           ALLOW_REPEAT, ka_zoom,        {.i =  1} },
	{ SDLK_b,       KMOD_LSHIFT, 0,            ka_change_tool, {.i = BRUSH_SQUARE} },
	{ SDLK_b,       0,           0,            ka_change_tool, {.i = BRUSH_ROUND} },
	{ SDLK_e,       0,           0,            ka_change_tool, {.i = ERASER} },
	{ SDLK_g,       0,           0,            ka_change_tool, {.i = BUCKET_FILL} },
	{ SDLK_LEFTBRACKET,  0,      ALLOW_REPEAT, ka_brush_size,  {.i = -1} },
	{ SDLK_RIGHTBRACKET, 0,      ALLOW_REPEAT, ka_brush_size,  {.i =  1} },
	{ SDLK_z,       KMOD_LCTRL,  ALLOW_REPEAT, ka_undo_redo,   {.i = -1} },
	{ SDLK_y,       KMOD_LCTRL,  ALLOW_REPEAT, ka_undo_redo,   {.i =  1} },
	{ SDLK_LALT,    0,           0,            ka_change_tool, {.i = COLOR_PICKER} },
	{ SDLK_s,       KMOD_LCTRL|KMOD_LSHIFT, 0, ka_save_file,   {.i = SAVE_AS} },
	{ SDLK_s,       KMOD_LCTRL,  0,            ka_save_file,   {.i = SAVE} },
	{ SDLK_o,       KMOD_LCTRL,  0,            ka_open_file,   {0} },
	{ SDLK_n,       KMOD_LCTRL,  0,            ka_new_file,    {0} },
	{ SDLK_q,       KMOD_LCTRL,  0,            ka_quit,        {0} },
};

static KeyAction const key_up_actions[] = {
	{ SDLK_e,    0, 0, ka_change_tool, {.i = -1} },
	{ SDLK_LALT, 0, 0, ka_change_tool, {.i = -1} },
	{ SDLK_g,    0, 0, ka_change_tool, {.i = -1} },
};

static void poll_events()
{
	// Reset io state
	for (size_t i = 0; i < LENGTH(just_clicked); ++i) {
		just_clicked[i] = false;
	}

	// TODO: Fix strange scroll wheel bug when using SDL_WaitEvent.
	SDL_WaitEvent(NULL);

	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			try_quit_application();
			break;
		case SDL_KEYDOWN:
			for (size_t i = 0; i < LENGTH(key_down_actions); ++i) {
				KeyAction a = key_down_actions[i];
				SDL_Keysym keysym = e.key.keysym;
				if (a.key == keysym.sym && (a.mod == 0 || (a.mod & keysym.mod) == a.mod) && (!e.key.repeat || (a.flag & ALLOW_REPEAT))) {
					a.func(a.arg, keysym.sym, keysym.mod);
					break;
				}
			}
			break;
		case SDL_KEYUP:
			for (size_t i = 0; i < LENGTH(key_up_actions); ++i) {
				KeyAction a = key_up_actions[i];
				SDL_Keysym keysym = e.key.keysym;
				if (a.key == keysym.sym && (a.mod == 0 || (a.mod & keysym.mod) == a.mod) && (!e.key.repeat || (a.flag & ALLOW_REPEAT))) {
					a.func(a.arg, keysym.sym, keysym.mod);
					break;
				}
			}
			break;
		case SDL_MOUSEWHEEL: {
			int y = (e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) ? -e.wheel.y : e.wheel.y;
			if (SDL_GetModState() & KMOD_LCTRL) {
				change_zoom(y);
			} else {
				brush_resize(&brush, y);
			}
			break;
		}
		case SDL_MOUSEBUTTONDOWN: {
			SDL_Keymod mod = SDL_GetModState();
			int button = e.button.button;
			just_clicked[button] = true;
			if (button == SDL_BUTTON_MIDDLE || ((mod & KMOD_LCTRL) && button == SDL_BUTTON_LEFT)) {
				set_cursor(SDL_SYSTEM_CURSOR_HAND);
				panning = true;
			} else if (!ui_wants_mouse) {
				drawing = true;
				active_button = button;
				tool_on_click(active_button);
			}
			break;
		}
		case SDL_MOUSEBUTTONUP:
			if (panning) {
				set_cursor(SDL_SYSTEM_CURSOR_ARROW);
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
				constrain_canvas();
			} else if (drawing) {
				tool_on_move();
			}
			break;
		}
	}
}

static void cleanup(void)
{
	TTF_CloseFont(font);
	TTF_Quit();

	brush_free(brush);
	canvas_free(canvas);

	SDL_DestroyTexture(checkerboard);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
}

int main(int argc, char *argv[])
{
	atexit(cleanup);

	if (SDL_Init(SDL_INIT_VIDEO) || TTF_Init()) {
		fatalSDL("Could not initialize SDL2");
	}

	font = TTF_OpenFontRW(SDL_RWFromConstMem(font_data, font_data_len), true, 16);
	if (font == NULL) {
		fatalSDL("Could not open font");
	}
	TTF_SetFontHinting(font, TTF_HINTING_LIGHT);

	win = SDL_CreateWindow("Pixel Art Editor", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_RESIZABLE);
	if (win == NULL) {
		fatalSDL("Could not create window");
	}

	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (ren == NULL) {
		// Try different flags
		ren = SDL_CreateRenderer(win, -1, 0);
		if (ren == NULL) {
			fatalSDL("Could not create renderer");
		}
	}

	canvas = canvas_create_with_background(60, 40, 0x00000000, ren);
	brush = brush_create(5, true);
	left_color = default_palette[0];
	right_color = default_palette[1];

	while (running) {
		poll_events();
		render_canvas(dark_theme);
		render_user_interface(dark_theme);
		SDL_RenderPresent(ren);
	}

	return EXIT_SUCCESS;
}
