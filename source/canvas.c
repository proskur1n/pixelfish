#include "canvas.h"
#include "util.h"

Canvas canvas_create_from_memory(int w, int h, Color *pixels, SDL_Renderer *ren)
{
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
	int width = 0;
	int height = 0;
	uint32_t *rgba = (uint32_t *) stbi_load(filepath, &width, &height, NULL, 4);
	if (rgba == NULL) {
		memset(out_result, 0, sizeof(*out_result));
		return stbi_failure_reason();
	}

	// Convert endianness
	for (int i = 0; i < width * height; ++i) {
		union {
			uint32_t ui;
			struct { uint8_t r, g, b, a; };
		} u = { rgba[i] };
		rgba[i] = (u.r << 24) | (u.g << 16) | (u.b << 8) | u.a;
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
