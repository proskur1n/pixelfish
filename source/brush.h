#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	int size;
	bool round;
	uint8_t *stencil; // Holds at least size*size elements
	int sbytes; // Number of bytes allocated for the stencil buffer.
} Brush;

// Returns a new brush with the specified parameters.
Brush brush_create(int size, bool round);

// Resizes the brush to the specified size. Size will be clamped to the minimum value of one.
void brush_set_size(Brush *brush, int new_size);

// Resizes the brush by the given amount. Final size will be clamped to the minimum value of one.
void brush_resize(Brush *brush, int delta);

// Enables or disables the round brush.
void brush_set_round(Brush *brush, bool round);

// Frees the data allocated for the specified brush.
void brush_free(Brush brush);
