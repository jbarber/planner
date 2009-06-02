/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003 Imendio AB
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include "planner-view.h"
#include "planner-conf.h"
#include "planner-print-dialog.h"

#define PLANNER_PRINT_CONFIG_FILE "planner-print-config"


static gboolean
ensure_dir (void)
{
	char *dir;

	dir = g_build_filename (g_get_home_dir (), ".gnome2", NULL);

	if (!g_file_test (dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
		if (g_mkdir (dir, 0755) != 0) {
			g_free (dir);
			return FALSE;
		}
	}

	g_free (dir);

	dir = g_build_filename (g_get_home_dir (), ".gnome2", "planner", NULL);

	if (!g_file_test (dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
		if (g_mkdir (dir, 0755) != 0) {
			g_free (dir);
			return FALSE;
		}
	}

	g_free (dir);

	return TRUE;
}

static gchar *
get_config_filename (void)
{
	if (!ensure_dir ()) {
		return NULL;
	}

	return g_build_filename (g_get_home_dir (),
				 ".gnome2", "planner",
				 PLANNER_PRINT_CONFIG_FILE,
				 NULL);
}


GtkPageSetup *
planner_print_dialog_load_page_setup (void)
{
	gboolean          success;
	gchar            *filename;
	GKeyFile         *key_file;
	GtkPageSetup     *page_setup = NULL;

	filename = get_config_filename ();
	if(filename) {
		key_file = g_key_file_new();

		success = g_key_file_load_from_file (key_file,
						     filename,
						     G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS,
						     NULL);
		g_free (filename);

		if (success) {
			page_setup = gtk_page_setup_new_from_key_file (key_file, NULL, NULL);
		}

		if (page_setup == NULL) {
			page_setup = gtk_page_setup_new ();
		}

		g_key_file_free (key_file);
	}

	return page_setup;
}

void
planner_print_dialog_save_page_setup (GtkPageSetup *page_setup)
{
	gint      fd;
	gchar    *str;
	gint      bytes, bytes_written;
	gchar    *filename;
	GKeyFile *key_file;

	g_return_if_fail (page_setup != NULL);

	filename = get_config_filename ();
	if (filename) {
		key_file = g_key_file_new ();
		g_key_file_load_from_file (key_file,
					   filename,
					   G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS,
					   NULL);
		gtk_page_setup_to_key_file (page_setup, key_file, NULL);
		str = g_key_file_to_data (key_file, NULL, NULL);
		g_key_file_free (key_file);

		fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
		g_free (filename);

		if (fd >= 0) {
			bytes = strlen (str);

		again:
			bytes_written = write (fd, str, bytes);
			if (bytes_written < 0 && errno == EINTR) {
				goto again;
			}

			close (fd);
		}

		g_free (str);
	}
}

GtkPrintSettings *
planner_print_dialog_load_print_settings (void)
{
	gboolean          success;
	gchar            *filename;
	GKeyFile         *key_file;
	GtkPrintSettings *settings = NULL;

	filename = get_config_filename ();
	if(filename) {
		key_file = g_key_file_new();

		success = g_key_file_load_from_file (key_file,
						     filename,
						     G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS,
						     NULL);
		g_free (filename);

		if (success) {
			settings = gtk_print_settings_new_from_key_file (key_file, NULL, NULL);
		}

		if (settings == NULL) {
			settings = gtk_print_settings_new ();
		}

		g_key_file_free (key_file);
	}

	return settings;
}

void
planner_print_dialog_save_print_settings (GtkPrintSettings *settings)
{
	gint      fd;
	gchar    *str;
	gint      bytes, bytes_written;
	gchar    *filename;
	GKeyFile *key_file;

	g_return_if_fail (settings != NULL);

	filename = get_config_filename ();
	if (filename) {
		key_file = g_key_file_new ();
		g_key_file_load_from_file (key_file,
					   filename,
					   G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS,
					   NULL);
		gtk_print_settings_to_key_file (settings, key_file, NULL);
		str = g_key_file_to_data (key_file, NULL, NULL);
		g_key_file_free (key_file);

		fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
		g_free (filename);

		if (fd >= 0) {
			bytes = strlen (str);

		again:
			bytes_written = write (fd, str, bytes);
			if (bytes_written < 0 && errno == EINTR) {
				goto again;
			}

			close (fd);
		}

		g_free (str);
	}
}

GtkWidget *
print_dialog_create_page (PlannerWindow *window,
			  GtkWidget     *dialog,
			  GList         *views)
{
	GtkWidget *outer_vbox, *vbox;
	GtkWidget *hbox;
	GtkWidget *w;
	GList     *l;
	GList     *buttons = NULL;
	gchar     *str;
	gboolean   state;

	outer_vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (outer_vbox), 8);

	str = g_strconcat ("<b>", _("Select the views to print:"), "</b>", NULL);
	w = gtk_label_new (str);
	g_free (str);
	gtk_box_pack_start (GTK_BOX (outer_vbox), w, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (w), TRUE);
	gtk_misc_set_alignment (GTK_MISC (w), 0, 0.5);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (outer_vbox), hbox, TRUE, TRUE, 0);

	w = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

/*	w = gtk_check_button_new_with_mnemonic (_("Project summary"));
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
	g_object_set_data (G_OBJECT (dialog), "summary-button", w);
*/

	for (l = views; l; l = l->next) {
		/* Hack. */
		if (strcmp (planner_view_get_name (l->data), "resource_usage_view") == 0) {
			continue;
		}

		w = gtk_check_button_new_with_label (planner_view_get_label (l->data));
		gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
		g_object_set_data (G_OBJECT (w), "view", l->data);

		str = g_strdup_printf ("/views/%s/print_enabled",
				       planner_view_get_name (l->data));
		state = planner_conf_get_bool (str, NULL);
		g_free (str);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), state);

		buttons = g_list_prepend (buttons, w);
	}

	g_object_set_data (G_OBJECT (outer_vbox), "buttons", buttons);

	gtk_widget_show_all (outer_vbox);

	return outer_vbox;
}

GList *
planner_print_dialog_get_print_selection (GtkWidget *widget,
					  gboolean  *summary)
{
	GtkToggleButton *button;
	GList           *buttons, *l;
	GList           *views = NULL;
	PlannerView     *view;
	gchar           *str;

	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

/*	button = g_object_get_data (G_OBJECT (dialog), "summary-button");
	if (summary) {
		*summary = gtk_toggle_button_get_active (button);
	}
*/

	buttons = g_object_get_data (G_OBJECT (widget), "buttons");
	for (l = buttons; l; l = l->next) {
		button = l->data;

		view = g_object_get_data (G_OBJECT (l->data), "view");

		if (gtk_toggle_button_get_active (button)) {
			views = g_list_prepend (views, view);
		}

		str = g_strdup_printf ("/views/%s/print_enabled",
				       planner_view_get_name (view));
		planner_conf_set_bool (str,
				       gtk_toggle_button_get_active (button),
				       NULL);
		g_free (str);
	}

	return views;
}

