#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <gtk/gtk.h>
#include "dialog.h"

static void initialize_gtk(void)
{
	static bool done = false;
	if (!done) {
		gtk_init(NULL, NULL);
		done = true;
	}
}

static char *show_file_chooser(char const *title, GtkFileChooserAction action)
{
	initialize_gtk();

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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
static void set_background_color(GtkWidget *widget, char const *color)
{
	GdkColor c;
	gdk_color_parse(color, &c);
	gtk_widget_modify_bg(widget, GTK_STATE_NORMAL, &c);
}
#pragma GCC diagnostic pop

DialogResponse dialog_unsaved_changes_confirmation(void)
{
	initialize_gtk();

	GtkWidget *dialog = gtk_dialog_new_with_buttons("Unsaved Changes", NULL, GTK_DIALOG_MODAL,
		"Discard", DIALOG_DISCARD, "Save", DIALOG_SAVE, "Cancel", GTK_RESPONSE_CANCEL, NULL);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 330, 110);
	gtk_window_set_keep_above(GTK_WINDOW(dialog), true);

	GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	g_object_set(content_area, "margin", 15, NULL);

	GtkWidget *label = gtk_label_new("You have unsaved changes that will be lost.");
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_container_add(GTK_CONTAINER(content_area), label);

	GtkWidget *discard_button = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), DIALOG_DISCARD);
	set_background_color(discard_button, "#e01b24");
	GtkWidget *cancel_button = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
	gtk_widget_grab_default(cancel_button);

	gtk_widget_show_all(dialog);
	gint res = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	while (gtk_events_pending()) {
		gtk_main_iteration();
	}

	if (res == DIALOG_DISCARD || res == DIALOG_SAVE) {
		return res;
	}
	return DIALOG_CANCEL;
}
