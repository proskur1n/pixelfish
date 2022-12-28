#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <gtk/gtk.h>
#include "dialog.h"

static bool initialized = false;

static char *show_file_chooser(char const *title, GtkFileChooserAction action)
{
	if (!initialized) {
		gtk_init(NULL, NULL);
		initialized = true;
	}

	GtkWidget *dialog = gtk_file_chooser_dialog_new(title, NULL, action,
		"_Cancel", GTK_RESPONSE_CANCEL,
		(action == GTK_FILE_CHOOSER_ACTION_OPEN ? "_Open" : "_Save"), GTK_RESPONSE_ACCEPT, NULL);

	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	gtk_file_chooser_set_select_multiple(chooser, false);
	gtk_file_chooser_set_do_overwrite_confirmation(chooser, true);

	char *filename = NULL;
	gint res = gtk_dialog_run(GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename(chooser);
	}

	gtk_widget_destroy(dialog);
	while (gtk_events_pending()) {
		gtk_main_iteration();
	}

	return filename;
}

char *dialog_open_file(char const *title)
{
	if (title == NULL) {
		title = "Open File";
	}
	return show_file_chooser(title, GTK_FILE_CHOOSER_ACTION_OPEN);
}

char *dialog_save_file(char const *title)
{
	if (title == NULL) {
		title = "Save File";
	}
	return show_file_chooser(title, GTK_FILE_CHOOSER_ACTION_SAVE);
}
