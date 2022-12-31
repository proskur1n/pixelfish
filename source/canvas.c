#include <assert.h>
#include "canvas.h"
#include "util.h"
#include "dialog.h"

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
	if (width == 0 || height == 0) {
		// No idea if this can actually happen :/
		free(rgba);
		return "Empty image";
	}

	// Convert endianness
	for (int i = 0; i < width * height; ++i) {
		rgba[i] = SDL_SwapBE32(rgba[i]);
	}

	*out_result = canvas_create_from_memory(width, height, rgba, ren);
	out_result->filepath = xstrdup(filepath);
	return NULL;
}

void canvas_free(Canvas c)
{
	if (c.texture != NULL) {
		SDL_DestroyTexture(c.texture);
	}
	free(c.pixels);
	free(c.pixels_backup);
	for (size_t i = 0; i < LENGTH(c.history); ++i) {
		free(c.history[i]);
	}
	free((char *) c.filepath);
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
	canvas->unsaved = true;
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

	for (int i = 0; i < canvas->redo_left; ++i) {
		int j = (canvas->next_hist + i) % MAX_UNDO_LENGTH;
		free(canvas->history[j]);
		canvas->history[j] = NULL;
	}
	canvas->redo_left = 0;
	free(canvas->history[canvas->next_hist]);
	canvas->history[canvas->next_hist] = up;
	canvas->next_hist = (canvas->next_hist + 1) % MAX_UNDO_LENGTH;
	if (canvas->undo_left < MAX_UNDO_LENGTH) {
		++canvas->undo_left;
	}
	memset(&canvas->dirty, 0, sizeof(canvas->dirty));
}

// Reverts any changes made to the dirty region after the last commit. The region itself will be
// reset to zero.
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

	if (canvas->next_hist == 0) {
		// Wrap over
		canvas->next_hist = MAX_UNDO_LENGTH - 1;
	} else {
		--canvas->next_hist;
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

int stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes);
int stbi_write_bmp(char const *filename, int w, int h, int comp, const void *data);
int stbi_write_tga(char const *filename, int w, int h, int comp, const void *data);
// TODO: Implement my own functions to read and write PPM files. This image format is not properly
// supported by stbi, but I find it useful because of its simplicity.

CanvasFileStatus canvas_save_to_file(Canvas *canvas, char const *filepath)
{
	if (!canvas->unsaved && filepath == NULL && canvas->filepath != NULL) {
		return CF_OK;
	}

	if (filepath == NULL && canvas->filepath == NULL) {
		filepath = dialog_save_file("Save File");
		if (filepath == NULL) {
			return CF_CANCELLED_BY_USER;
		}
	} else {
		filepath = xstrdup(filepath == NULL ? canvas->filepath : filepath);
	}

	if (SDL_BYTEORDER == SDL_LIL_ENDIAN) {
		// stbi expects the image data to be in the RGBA order. Therefore, we must do a conversion
		// because we store pixels as endianness-dependent numbers.
		for (int i = 0; i < canvas->w * canvas->h; ++i) {
			canvas->pixels[i] = SDL_Swap32(canvas->pixels[i]);
		}
	}

	int success = 0;
	int unknown_format = 0;
	char const *ext = strrchr(filepath, '.');
	int const comp = 4; // RGBA
	if (ext == NULL || strcmp(ext, ".png") == 0) {
		success = stbi_write_png(filepath, canvas->w, canvas->h, comp, canvas->pixels, 0);
	} else if (strcmp(ext, ".bmp") == 0 || strcmp(ext, ".dib") == 0) {
		success = stbi_write_bmp(filepath, canvas->w, canvas->h, comp, canvas->pixels);
	} else if (strcmp(ext, ".tga") == 0) {
		success = stbi_write_tga(filepath, canvas->w, canvas->h, comp, canvas->pixels);
	} else {
		unknown_format = 1;
	}

	if (SDL_BYTEORDER == SDL_LIL_ENDIAN) {
		// Swap the bytes back to their original ordering.
		for (int i = 0; i < canvas->w * canvas->h; ++i) {
			canvas->pixels[i] = SDL_Swap32(canvas->pixels[i]);
		}
	}

	if (success) {
		free((char *) canvas->filepath);
		canvas->filepath = filepath;
		canvas->unsaved = false;
		return CF_OK;
	}

	free((char *) filepath); // Filepath is now a copy. We have to free it.
	if (unknown_format) {
		return CF_UNKNOWN_IMAGE_FORMAT;
	}
	return CF_OTHER_ERROR;
}

CanvasFileStatus canvas_save_as_to_file(Canvas *canvas)
{
	char *filepath = dialog_save_file("Save File As");
	if (filepath == NULL) {
		return CF_CANCELLED_BY_USER;
	}
	CanvasFileStatus status = canvas_save_to_file(canvas, filepath);
	free(filepath); // canvas_save_to_file creates a copy of filepath.
	return status;
}
