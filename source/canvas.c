#include "canvas.h"
#include "util.h"

Canvas *canvas_create_from_memory(int w, int h, Color *pixels, SDL_Renderer *ren)
{
	Canvas *c = calloc(1, sizeof(Canvas));
	Color *backup = memdup(pixels, w * h * sizeof(Color));
	SDL_Texture *texture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, w, h);

	if (!c || !backup || !texture) {
		free(pixels);
		free(c);
		free(backup);
		if (texture) {
			SDL_DestroyTexture(texture);
		}
		return NULL;
	}

	c->w = w;
	c->h = h;
	c->texture = texture;
	c->pixels = pixels;
	c->pixels_backup = backup;

	int pitch = w * sizeof(Color);
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	SDL_UpdateTexture(texture, NULL, pixels, pitch);
	return c;
}

Canvas *canvas_create_with_background(int w, int h, Color bg, SDL_Renderer *ren)
{
	Color *pixels = malloc(w * h * sizeof(Color));
	if (!pixels) {
		return NULL;
	}
	for (int i = 0; i < w * h; ++i) {
		pixels[i] = bg;
	}
	return canvas_create_from_memory(w, h, pixels, ren);
}

void canvas_free(Canvas *c)
{
	if (c != NULL) {
		free(c->pixels);
		free(c->pixels_backup);
		free(c);
	}
}
