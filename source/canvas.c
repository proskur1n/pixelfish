#include "canvas.h"
#include "util.h"

Canvas canvas_create_from_memory(int w, int h, Color *pixels, SDL_Renderer *ren)
{
	SDL_Texture *texture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, w, h);
	if (!texture) {
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

void canvas_free(Canvas c)
{
	free(c.pixels);
	free(c.pixels_backup);
}
