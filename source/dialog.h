#pragma once

// Shows a file chooser window with the specified title to the user. You can pass NULL for a
// default title. Returns NULL if the dialog was closed without selecting a file. The returned
// string must be freed by the caller.
char *dialog_open_file(char const *title);

// Shows a file save dialog with the specified title to the user. You can pass NULL for a default
// title. Returns NULL if the dialog was closed without selecting a file. The returned string must
// be freed by the caller.
char *dialog_save_file(char const *title);

typedef enum {
	DIALOG_CANCEL,
	DIALOG_SAVE,
	DIALOG_DISCARD,
} DialogResponse;

// Shows the user a confirmation dialog for "unsaved changes" with three generic choices.
DialogResponse dialog_unsaved_changes_confirmation(void);
