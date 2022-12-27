#include <assert.h>
#include "canvas.h"
#include "util.h"

Canvas canvas_create_from_memory(int w, int h, Color *pixels, SDL_Renderer *ren)
{
	assert(w > 0 && h > 0);
	SDL_Texture *texture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, w, h);
	if (texture == NULL) {
		fatalSDL("SDL_CreateTexture");
	}
	int pitch = w * sizeof(Color);
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	SDL_UpdateTexture(texture, NULL, pixels, pitch);

	return (Canvas) {
		.w = w,
		.h = h,
		.texture = texture,
		.pixels = pixels,
		.pixels_backup = xmemdup(pixels, w * h * sizeof(Color))
	};
}

Canvas canvas_create_with_background(int w, int h, Color bg, SDL_Renderer *ren)
{
	assert(w > 0 && h > 0);
	Color *pixels = xalloc(w * h * sizeof(Color));
	for (int i = 0; i < w * h; ++i) {
		pixels[i] = bg;
	}
	return canvas_create_from_memory(w, h, pixels, ren);
}

// Define these functions here to not include the huge header file.
typedef unsigned char stbi_uc;
stbi_uc *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
const char *stbi_failure_reason(void);

char const *canvas_open_image(Canvas *out_result, char const *filepath, SDL_Renderer *ren)
{
	memset(out_result, 0, sizeof(*out_result));
	int width = 0;
	int height = 0;
	uint32_t *rgba = (uint32_t *) stbi_load(filepath, &width, &height, NULL, 4);
	if (rgba == NULL) {
		return stbi_failure_reason();
	}

	// Convert endianness
	for (int i = 0; i < width * height; ++i) {
		// TODO: Use SDL_Swap* functions
		union {
			uint32_t ui;
			struct { uint8_t r, g, b, a; };
		} u = { rgba[i] };
		rgba[i] = (u.r << 24) | (u.g << 16) | (u.b << 8) | u.a;
	}

	if (width == 0 || height == 0) {
		// No idea if this can actually happen :/
		free(rgba);
		return "Empty image";
	}

	*out_result = canvas_create_from_memory(width, height, rgba, ren);
	return NULL;
}

void canvas_free(Canvas c)
{
	free(c.pixels);
	free(c.pixels_backup);
	if (c.texture != NULL) {
		SDL_DestroyTexture(c.texture);
	}
}

// Uploads pixels in the specified region to the texture.
static void update_texture(Canvas *canvas, SDL_Rect rect)
{
	unsigned char *tex_data = NULL;
	int pitch = 0; // Row length in bytes
	if (SDL_LockTexture(canvas->texture, &rect, (void **) &tex_data, &pitch) != 0) {
		fatalSDL("SDL_LockTexture");
	}
	for (int y = rect.y; y < rect.y + rect.h; ++y) {
		void *dest = &tex_data[(y - rect.y) * pitch];
		void *src = &canvas->pixels[y * canvas->w + rect.x];
		memcpy(dest, src, rect.w * sizeof(Color));
	}
	SDL_UnlockTexture(canvas->texture);
}

void canvas_mark_dirty(Canvas *canvas, SDL_Rect region)
{
	// Normalize negative side length. SDL_Rect functions do not handle them well.
	if (region.w < 0) {
		region.x += region.w;
		region.w = -region.w;
	}
	if (region.h < 0) {
		region.y += region.h;
		region.h = -region.h;
	}

	SDL_Rect bounds = {0, 0, canvas->w, canvas->h};
	if (SDL_IntersectRect(&region, &bounds, &region)) {
		// Note that this branch is not entered if the provided region is empty.
		SDL_UnionRect(&canvas->dirty, &region, &canvas->dirty);
		update_texture(canvas, region);
	}
}

struct UndoPoint {
	SDL_Rect affected;
	Color data[];
};

void canvas_commit(Canvas *canvas)
{
	SDL_Rect dirty = canvas->dirty;
	if (dirty.w == 0 || dirty.h == 0) {
		// Do not commit empty regions.
		return;
	}
	int npixels = dirty.w * dirty.h;
	assert(npixels > 0);

	UndoPoint *up = xalloc(sizeof(UndoPoint) + npixels * sizeof(Color));
	up->affected = dirty;
	for (int y = dirty.y, i = 0; y < dirty.y + dirty.h; ++y) {
		for (int x = dirty.x; x < dirty.x + dirty.w; ++x) {
			int ci = y * canvas->w + x;
			assert(i < npixels);
			up->data[i++] = canvas->pixels_backup[ci];
			canvas->pixels_backup[ci] = canvas->pixels[ci];
		}
	}

	canvas->history[canvas->next_hist] = up;
	canvas->next_hist = (canvas->next_hist + 1) % MAX_UNDO_LENGTH;
	if (canvas->undo_left < MAX_UNDO_LENGTH) {
		++canvas->undo_left;
	}
	canvas->redo_left = 0;
	memset(&canvas->dirty, 0, sizeof(canvas->dirty));
}

// Reverts an uncommited dirty region to its initial state. Dirty region will be reset to zero.
static void revert_uncommited_changes(Canvas *canvas)
{
	SDL_Rect dirty = canvas->dirty;
	if (dirty.w != 0 && dirty.h != 0) {
		for (int y = dirty.y; y < dirty.y + dirty.h; ++y) {
			for (int x = dirty.x; x < dirty.x + dirty.w; ++x) {
				int i = y * canvas->w + x;
				canvas->pixels[i] = canvas->pixels_backup[i];
			}
		}
		update_texture(canvas, dirty);
	}
	memset(&canvas->dirty, 0, sizeof(canvas->dirty));
}

// Swaps pixels in the region specified by up->affected between the canvas's buffer and the undo
// point's data.
static void swap_pixels(Canvas *canvas, UndoPoint *up)
{
	int i = 0;
	for (int y = up->affected.y; y < up->affected.y + up->affected.h; ++y) {
		for (int x = up->affected.x; x < up->affected.x + up->affected.w; ++x) {
			Color temp = canvas->pixels[y * canvas->w + x];
			canvas->pixels[y * canvas->w + x] = up->data[i];
			canvas->pixels_backup[y * canvas->w + x] = up->data[i];
			up->data[i++] = temp;
		}
	}
	update_texture(canvas, up->affected);
}

bool canvas_undo(Canvas *canvas)
{
	assert(canvas->undo_left >= 0 && canvas->undo_left <= MAX_UNDO_LENGTH);
	assert(canvas->redo_left >= 0 && canvas->redo_left <= MAX_UNDO_LENGTH);

	revert_uncommited_changes(canvas);

	if (canvas->undo_left == 0) {
		return false;
	}
	--canvas->undo_left;
	++canvas->redo_left;

	canvas->next_hist = (canvas->next_hist - 1) % MAX_UNDO_LENGTH;
	if (canvas->next_hist == -1) {
		// Wrap over
		canvas->next_hist = MAX_UNDO_LENGTH - 1;
	}
	swap_pixels(canvas, canvas->history[canvas->next_hist]);
	return true;
}

bool canvas_redo(Canvas *canvas)
{
	assert(canvas->undo_left >= 0 && canvas->undo_left <= MAX_UNDO_LENGTH);
	assert(canvas->redo_left >= 0 && canvas->redo_left <= MAX_UNDO_LENGTH);

	revert_uncommited_changes(canvas);

	if (canvas->redo_left == 0) {
		return false;
	}
	++canvas->undo_left;
	--canvas->redo_left;

	swap_pixels(canvas, canvas->history[canvas->next_hist]);
	canvas->next_hist = (canvas->next_hist + 1) % MAX_UNDO_LENGTH;
	return true;
}
