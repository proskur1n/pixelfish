// Canvas is only responsible for storing its pixels and managing the undo/redo
// system. It does not implement any drawing operations.
#pragma once

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_rect.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint32_t Color; // RGBA

#define RED(x) ((x) >> 24)
#define GREEN(x) (((x) >> 16) & 0xff)
#define BLUE(x) (((x) >> 8) & 0xff)
#define ALPHA(x) ((x) & 0xff)

// Converts SDL_Color to Color
#define COLOR_FROM(c) ((c.r << 24) | (c.g << 16) | (c.b << 8) | c.a)

enum { MAX_UNDO_LENGTH = 64 };

typedef struct UndoPoint UndoPoint;

typedef struct {
	int w;
	int h;
	SDL_Texture *texture;
	Color *pixels; // [w * h] pixels in RGBA format.
	Color *pixels_backup; // Used inside the history system. Do not edit directly.
	UndoPoint *history[MAX_UNDO_LENGTH]; // Circular buffer
	int next_hist; // Next index inside history
	int undo_left; // Remaining amount of undo steps (<= MAX_UNDO_LENGTH)
	int redo_left; // Amount of redo operations left. Every successful undo increments this counter.
	SDL_Rect dirty; // Difference between pixels and pixels_backup
	char const *filepath; // Can be NULL if this canvas has not been associated with a file yet. Allocated on the heap.
	bool unsaved;
} Canvas;

// Pixels must point to a valid heap-allocated [w * h] array. Canvas becomes
// the sole owner of this memory buffer. Do not free pixels yourself.
Canvas canvas_create_from_memory(int w, int h, Color *pixels, SDL_Renderer *ren);

// Creates a new canvas with bg as its background.
Canvas canvas_create_with_background(int w, int h, Color bg, SDL_Renderer *ren);

// Tries to create a canvas with the specified image content. Returns NULL on success and an error
// message on failure. Note that the previous canvas inside 'out_result' is NOT free'd by this
// function.
char const *canvas_open_image(Canvas *out_result, char const *filepath, SDL_Renderer *ren);

// Frees the dynamically allocated memory inside the canvas.
void canvas_free(Canvas c);

// Marks the given region as dirty. Marking the same area as dirty multiple times has no effect.
// Dirty regions will be added to the undolist by the next commit on this canvas. This function
// will also update the underlying texture buffer.
void canvas_mark_dirty(Canvas *canvas, SDL_Rect region);

// Adds a new savepoint to the undolist containing all currently dirty regions. Clears the dirty
// status of all added regions. If the dirty rectangle is empty then no commit is done.
void canvas_commit(Canvas *canvas);

// Undoes the last fully commited operation. Returns false if no history was left and no undo
// was performed. This operation will cancel any uncommited changes to the canvas. This function
// will also update the underlying texture buffer.
bool canvas_undo(Canvas *canvas);

// Reverts an undo operation. Returns false if no history was left and no redo was performed. This
// operation will cancel any uncommited changes to the canvas. This function will also update the
// underlying texture buffer.
bool canvas_redo(Canvas *canvas);

typedef enum {
	CF_OK, // Everything went as planned.
	CF_CANCELLED_BY_USER, // User has canceled a file chooser dialog.
	CF_UNKNOWN_IMAGE_FORMAT, // The specified filepath has an unknown image extension.
	CF_OTHER_ERROR,
} CanvasFileStatus;

// Tries to save the image data inside the canvas to a file. If filepath is not NULL then this
// canvas becomes associated with it. Otherwise, the user may be prompted to enter a filepath if
// this canvas has not yet been associated with a file.
CanvasFileStatus canvas_save_to_file(Canvas *canvas, char const *filepath);

// This function is similar to canvas_save_to_file, but the user is always prompted to enter a
// filename.
CanvasFileStatus canvas_save_as_to_file(Canvas *canvas);
