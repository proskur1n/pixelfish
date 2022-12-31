#pragma once

#include <stdbool.h>

// Shows a file chooser window with the specified title to the user. You can pass NULL for a
// default title. Returns NULL if the dialog was closed without selecting a file. The returned
// string must be freed by the caller.
char *dialog_open_file(char const *title);

// Shows a file save dialog with the specified title to the user. You can pass NULL for a default
// title. Returns NULL if the dialog was closed without selecting a file. The returned string must
// be freed by the caller.
char *dialog_save_file(char const *title);

typedef enum {
	DIALOG_RESPONSE_CANCEL,
	DIALOG_RESPONSE_SAVE,
	DIALOG_RESPONSE_DISCARD,
} DialogResponse;

// Shows the user a confirmation dialog for "unsaved changes" with three generic choices.
DialogResponse dialog_unsaved_changes_confirmation(void);

// Prompts the user to enter the new image width and height in pixels. Returns false if the dialog
// was cancelled. Note that this function does not validate the entered dimensions.
bool dialog_width_and_height(int *out_width, int *out_height);
